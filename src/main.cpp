#include <winsock2.h>
#include <v8.h>
#include <node.h>
#include "nodeutils.h"
#include "nodeice.h"

#pragma comment(lib, "node")

v8::Persistent<v8::String> g_v8s_type = v8::Persistent<v8::String>::New(v8::String::New("__type"));
v8::Persistent<v8::String> g_v8s_subtype = v8::Persistent<v8::String>::New(v8::String::New("subtype"));
v8::Persistent<v8::String> g_v8s_optional = v8::Persistent<v8::String>::New(v8::String::New("optional"));
v8::Persistent<v8::String> g_v8s_length = v8::Persistent<v8::String>::New(v8::String::New("length"));
v8::Persistent<v8::String> g_v8s_write = v8::Persistent<v8::String>::New(v8::String::New("write"));
v8::Persistent<v8::String> g_v8s_read = v8::Persistent<v8::String>::New(v8::String::New("read"));

static void init(v8::Handle<v8::Object> target) 
{
	ice_node_register(target);
}

NODE_MODULE(ice, init);
