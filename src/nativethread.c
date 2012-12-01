#include "nativethread-priv.h"

typedef struct Proc {
  lua_State *L;
  pthread_t thread;
  pthread_cond_t cond;
  const char *channel;
  struct Proc *previous;
  struct Proc *next;
} Proc;

static Proc *waitsend = NULL;
static Proc *waitreceive = NULL;
static pthread_mutex_t kernel_access = PTHREAD_MUTEX_INITIALIZER;

static Proc *getself(lua_State *L) {
  Proc *p;
  lua_getfield(L, LUA_REGISTRYINDEX, "_SELF");
  p = (Proc *)lua_touserdata(L, -1);
  lua_pop(L, 1);
  return p;
}

static void movevalues(lua_State *send, lua_State *recv) {
  int n = lua_gettop(send);
  /* value at stack #1 is channel. */
  int i;
  for (i = 2; i <=n; i++)
    lua_pushstring(recv, lua_tostring(send, i));
}

static Proc *searchmatch(const char *channel, Proc **list) {
  Proc *node = *list;
  if (node == NULL)
    return NULL;
  do {
    if (strcmp(channel, node->channel) == 0) {
      if (*list == node)
        *list = (node->next == node) ? NULL : node->next;
      node->previous->next = node->next;
      node->next->previous = node->previous;
      return node;
    }
    node = node->next;
  } while (node != *list);
  return NULL;
}

static void waitonlist(lua_State *L, const char *channel, Proc **list) {
  Proc *p = getself(L);
  if (*list == NULL) {
    *list = p;
    p->previous = p->next = p;
  } else {
    p->previous = (*list)->previous;
    p->next = *list;
    p->previous->next = p->next->previous = p;
  }

  p->channel = channel;
  do {
    pthread_cond_wait(&p->cond, &kernel_access);
  } while (p->channel);
}

static int ll_send(lua_State *L) {
  Proc *p;
  const char *channel = luaL_checkstring(L, 1);

  pthread_mutex_lock(&kernel_access);

  p = searchmatch(channel, &waitreceive);
  if (p) {
    movevalues(L, p->L);
    p->channel = NULL;
    pthread_cond_signal(&p->cond);
  } else
    waitonlist(L, channel, &waitsend);

  pthread_mutex_unlock(&kernel_access);
  return 0;
}

static int ll_receive(lua_State *L) {
  Proc *p;
  const char *channel = luaL_checkstring(L, 1);
  lua_settop(L, 1);

  pthread_mutex_lock(&kernel_access);

  p = searchmatch(channel, &waitsend);
  if (p) {
    movevalues(p->L, L);
    p->channel = NULL;
    pthread_cond_signal(&p->cond);
  } else
    waitonlist(L, channel, &waitreceive);

  pthread_mutex_unlock(&kernel_access);

  return lua_gettop(L) - 1;
}

static void registerlibs(lua_State *L, luaL_Reg *libs) {
  luaL_Reg *reg;
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  for (reg = libs; reg->name; ++reg) {
    lua_pushcfunction(L, reg->func);
    lua_setfield(L, -2, reg->name);
  }
  lua_pop(L, 2);
}

static luaL_Reg libs[] = {
  { "io", luaopen_io },
  { "os", luaopen_os },
  { "table", luaopen_table },
  { "string", luaopen_string },
  { "math", luaopen_math },
  { "debug", luaopen_debug },
  { NULL, NULL }
};

static void openlibs(lua_State *L) {
#if LUA_VERSION_NUM == 502
  luaL_requiref(L, "base",    luaopen_base, 0);
  luaL_requiref(L, "package", luaopen_package, 1);
  lua_pop(L, 2);
#elif LUA_VERSION_NUM == 501
  lua_cpcall(L, luaopen_base, NULL);
  lua_cpcall(L, luaopen_package, NULL);
#else
unsupported_lua_version
#endif

  registerlibs(L, libs);
}

static void *ll_thread(void *arg) {
  lua_State *L = (lua_State *)arg;
  openlibs(L);

  /* open nativethread library */
  lua_pushcfunction(L, luaopen_nativethread);
  lua_pcall(L, 0, 0, 0);

  /* call main chunk passed as a string argument */
  if (lua_pcall(L, 0, 0, 0) != 0)
    fprintf(stderr, "thread error: %s", lua_tostring(L, -1));
  pthread_cond_destroy(&getself(L)->cond);
  lua_close(L);
  return NULL;
}

static int ll_start(lua_State *L) {
  pthread_t thread;
  const char *chunk = luaL_checkstring(L, 1);
  lua_State *L1 = luaL_newstate();
  if (L1 == NULL)
    luaL_error(L, "unable to create new state");

  if (luaL_loadstring(L1, chunk) != 0)
    luaL_error(L, "error starting thread: %s", lua_tostring(L1, -1));

  if (pthread_create(&thread, NULL, ll_thread, L1) != 0)
    luaL_error(L, "unable to create new thread");

  pthread_detach(thread);
  return 0;
}

static int ll_exit(lua_State *L) {
  pthread_exit(NULL);
  return 0;
}

static const struct luaL_Reg functions[] = {
  { "start", ll_start },
  { "send", ll_send },
  { "receive", ll_receive },
  { "exit", ll_exit },
  { NULL, NULL }
};

static void setfuncs(lua_State *L, const luaL_Reg *l) {
  for (; l->name; ++l) {
    lua_pushcfunction(L, l->func);
    lua_setfield(L, -2, l->name);
  }
}

int luaopen_nativethread(lua_State *L) {
  Proc *self = (Proc *)lua_newuserdata(L, sizeof(Proc));
  lua_setfield(L, LUA_REGISTRYINDEX, "_SELF");
  self->L = L;
  self->thread = pthread_self();
  self->channel = NULL;
  pthread_cond_init(&self->cond, NULL);

  lua_newtable(L);
  setfuncs(L, functions);

  return 1;
}
