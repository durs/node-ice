#include <winsock2.h>
#include <v8.h>
#include <node.h>
#include "nodeutils.h"
#include "nodeice.h"

#pragma comment(lib, "node")

static void init(v8::Handle<v8::Object> target) 
{
	ice_register(target);
}

NODE_MODULE(ice, init);
