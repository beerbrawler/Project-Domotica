var net = require('net');

var client = new net.Socket();
client.connect(3000, '192.168.1.100', function () {
	console.log('Connected');
	client.write(new Buffer([97, 4, 5]));
});

client.on('data', function (data) {
	console.log('Received: ' + data);
});

client.on('close', function () {
	console.log('Bye!');
});