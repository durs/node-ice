
::WARNING:: Partial ZeroC Ice RPC client implementation

# node-ice

This project provides a bridge to the ZeroC ICE RPC framework.

Project tested only on Windows platform (Visual Studio 2010) with Node.JS 0.10.1, ZeroC Ice 3.5.0.
	
What follows is a simple example.

Server slice declarations:

	module Monitoring
	{
		sequence<byte> ByteArray;

		struct Options
		{
			long params;
			string userid;
			double modifytime;
		};

		interface Service
		{
			void shutdown();
			idempotent string echo(string msg, optional(1) int delay, optional(2) Options opt);
		};
	}

Ice client node.js implementation code:
	
	var ice = require('ice');

	/**
	* javascript analog slice declarations
	*/
	var Monitoring = (function(){
		var ByteArray = ice.Sequence('byte');
		var Options = {
			params: 'long',
			userid: 'string',
			modifytime: 'double'
		}
		var Service = {
			shutdown: ice.Method(),
			echo: ice.Method('string', ice.mode.idempotent, [
				ice.Argument('msg', 'string'), 
				ice.Argument('delay', 'int', 1),
				ice.Argument('opt', Options, 2)
			])
		}
		return {
			ByteArray: ByteArray,
			Options: Options,
			Service: Service
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
	* examples:
	*     // from configuration property
	*     communicator.propertyToProxy('Storage.Proxy', Monitoring.Service);												
	*     // from string
	*     communicator.stringToProxy('service:tcp -h localhost -p 10000:udp -h localhost -p 10000', Monitoring.Service);	
	*/
	var service = communicator.propertyToProxy('Storage.Proxy', Monitoring.Service);

	/**
	* invoke ice object methods
	* examples: 
	*    service.echo("Hello"); 
	*    service.echo("Hello", 5000, {userid:'guest'});
	*    service.echo("Hello", null, [0, 'guest']));
	*/
	var result = service.echo("Hello", 1000);
	service.shutdown();



ToDo: 

	- Application terminated in release mode; 

	- Implemets ice class & ice user exception; 

	- Test optional parameters: with sequence, with dictonary, at the begin of definition, on reordering tags;

	- Automatically parsing slice file into javascript

	- Implements server features

	- Search memory leaks;
