
::WARNING:: Partial ZeroC Ice client implementation

# node-ice

This project provides a bridge to the ZeroC ICE RPC framework.
Tested only with Node.js v 0.10.1 on Windows platform (Visual Studio 2010)

What follows is a simple example. Note that this hasn't yet even been fully implemented but this should give you an idea of what we're aiming for.
Server slice declations:

	module Monitoring
	{
		exception Error
		{
			int code;
			string message;
		};
	
		struct Options
		{
			long params;
			string userid;
		};

		interface Storage
		{
			["ami", "amd"] idempotent string info() throws Error;
			["ami", "amd"] idempotent string format(string name, Options opt, string fmt) throws Error;			
		};
	}

Node.js client implementation code:
	
	var ice = require('ice');

	/**
	* javascript analog slice declarations
	*/
	var Monitoring = (function(){
		var Error = {
			code: 'int',
			message: 'string'
		}
		var Options = {
			params: 'long',
			userid: 'string',
		}	
		var Storage = {
			info: { mode: ice.mode.idempotent, result: 'string', error: Error },
			format: { mode: ice.mode.idempotent, result: 'string', error: Error, args:{
				name: 'string',
				opt: Options,
				fmt: 'string'
			}}
		}
		return {
			Error: Error,
			Options: Options,
			Storage: Storage
		}
	})();
	
	/**
	* initialize ice communicator
	*/
	var communicator = 
		// from parameters
		new ice.Communicator({
			"Storage.Proxy": "service:tcp -h localhost -p 10000:udp -h localhost -p 10000",
			"Ice.Trace.Network": 1,
			"Ice.Trace.Protocol": 1
		});
		
		// or from configuration file
		//new ice.Communicator('./test.conf');
		
	/**
	* create ice object proxy 
	*/
	var storage = 
		// from configuration property
		communicator.propertyToProxy('Storage.Proxy', Monitoring.Storage);
		
		// or from string
		//communicator.stringToProxy('service:tcp -h localhost -p 10000:udp -h localhost -p 10000', Monitoring.Storage);

	/**
	* invoke ice object methods
	*/
	console.log('Storage.info: ' + storage.info());
	console.log('Storage.format: ' + storage.format('test.users', {params: 1}, 'json'));
