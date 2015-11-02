# node-pdfium-native
Native port of [PDFium](https://code.google.com/p/pdfium/) to [node.js](https://nodejs.org/), [iojs](https://iojs.org/en/index.html) and [Nw.js](http://nwjs.io/).
The main motivation of this project is to render/convert a `PDF` format direct into `EMF` format, to be able to print on windows printer by using native [printer module](https://github.com/tojocky/node-printer).

# Methods:
* `getSupportedOutputFormats()` - returns an array of all supported output formats: `['BPM', 'EMF', 'PNG', 'PPM']`. Note that `EMF` and `BMP` formats are supported only on windows.
* `render(options, [callback(err, pages)])` - render/convert a PDF data into one of output format from `getSupportedOutputFormats()`.
Parameters:
  * `option` (object, mandatory) may contains the following fields:
    * `data` (Buffer, mandatory) - PDF data buffer
    * `outputFormat` (String, mandatory) - output format name. one from `getSupportedOutputFormats()`
    * `scaleFactor` (Number, optional, default: 1.0) - scale factor of the output format.
  *  `callback(err, pages)` (function, optional) - callback function. `pages` is array of buffers. If callback is missing, then the `pages` will be returned from `render(options)`. In case of error, an exception will be thrown.

# License:
`BSD` - feel free to use and support.
