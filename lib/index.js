var native_module = {},
    fs = require("fs"),
    os = require("os"),
    path = require("path"),
    native_lib_path = path.join(__dirname, '../build/Release/node_pdfium.node');

if(fs.existsSync(native_lib_path)) {
    native_module = require(native_lib_path);
} else {
    native_module = require('./node_printer_'+process.platform+'_'+process.arch+'.node');
}

module.exports.getSupportedOutputFormats = native_module.getSupportedOutputFormats;

module.exports.render = native_module.render;