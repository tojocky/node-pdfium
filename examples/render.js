var pdfium = require('../lib'), // in your case require('pdfium');
	util = require('util'),
	fs = require('fs'),
	path = require('path'),
	os = require('os'),
	options = {
		data: fs.readFileSync(path.join(__dirname, 'pdf-sample.pdf')),
		outputFormat: process.argv[2] || 'PNG' // could be a item from pdfium.getSupportedOutputFormats();
	}; 

console.log('converting PDF to ', options.outputFormat);
//async
console.log('run async');
pdfium.render(options, function(err, data) {
	if(err) {
		return console.error('error from async convert: ', err);
	}
	console.log('result from async:');
	printPages(data);
})

// sync
try {
	console.log('run sync');
	var data = pdfium.render(options);
	console.log('result from sync: ')
	printPages(data);
} catch(err) {
	console.log('error from sync:', err);
}

function printPages(iPages) {
	if(!iPages) {
		return console.error('no pages');
	}
	if(typeof(iPages) !== 'object' || (!(iPages instanceof Array))) {
		return console.error('Pages has different type than Array (' + typeof(iPages) + ')');
	}
	var i;
	for(i = 0; i < iPages.length; ++i) {
		data = iPages[i];
		if(data instanceof Buffer) {
			var filename = path.join(os.tmpdir(), 'pdfoutput_' + (i+1) + '-' + iPages.length + '.'+options.outputFormat);
			fs.writeFileSync(filename, data);
			console.log('page ' + (i+1) + '(buffer) size: '+data.length+'B: ' + data.toString('base64').substr(0, 30) + '... saved to ' + filename);
		} else {
			console.log('page ' + (i+1) + '(' + typeof(data) + '):' + data.toString('base64').substr(0, 10))
		}
	}
}