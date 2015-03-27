var pdfium = require('../lib'), // in your case require('pdfium');
	util = require('util');

var supportedFormats = pdfium.getSupportedOutputFormats();
console.log('Supported output formats: ', util.inspect(supportedFormats, {colors:true, depth:10}));