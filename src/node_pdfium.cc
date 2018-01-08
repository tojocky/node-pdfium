#include <node.h>

#include "node_pdfium.hh"

#include <uv.h>
#include <node_buffer.h>

#include <string>
#include <map>
#include <vector>
#include <utility>
#include <sstream>

#include "../third_party/pdfium/fpdfsdk/include/fpdf_dataavail.h"
#include "../third_party/pdfium/fpdfsdk/include/fpdf_ext.h"
#include "../third_party/pdfium/fpdfsdk/include/fpdfformfill.h"
#include "../third_party/pdfium/fpdfsdk/include/fpdftext.h"
#include "../third_party/pdfium/fpdfsdk/include/fpdfview.h"
#include "../third_party/pdfium/core/include/fxcrt/fx_system.h"

#include "image_diff_png.hh"

namespace {

void ExampleUnsupportedHandler(UNSUPPORT_INFO*, int type) {
  std::string feature = "Unknown";
  switch (type) {
    case FPDF_UNSP_DOC_XFAFORM:
      feature = "XFA";
      break;
    case FPDF_UNSP_DOC_PORTABLECOLLECTION:
      feature = "Portfolios_Packages";
      break;
    case FPDF_UNSP_DOC_ATTACHMENT:
    case FPDF_UNSP_ANNOT_ATTACHMENT:
      feature = "Attachment";
      break;
    case FPDF_UNSP_DOC_SECURITY:
      feature = "Rights_Management";
      break;
    case FPDF_UNSP_DOC_SHAREDREVIEW:
      feature = "Shared_Review";
      break;
    case FPDF_UNSP_DOC_SHAREDFORM_ACROBAT:
    case FPDF_UNSP_DOC_SHAREDFORM_FILESYSTEM:
    case FPDF_UNSP_DOC_SHAREDFORM_EMAIL:
      feature = "Shared_Form";
      break;
    case FPDF_UNSP_ANNOT_3DANNOT:
      feature = "3D";
      break;
    case FPDF_UNSP_ANNOT_MOVIE:
      feature = "Movie";
      break;
    case FPDF_UNSP_ANNOT_SOUND:
      feature = "Sound";
      break;
    case FPDF_UNSP_ANNOT_SCREEN_MEDIA:
    case FPDF_UNSP_ANNOT_SCREEN_RICHMEDIA:
      feature = "Screen";
      break;
    case FPDF_UNSP_ANNOT_SIG:
      feature = "Digital_Signature";
      break;
  }
  printf("Unsupported feature: %s.\n", feature.c_str());
}

int ExampleAppAlert(IPDF_JSPLATFORM*, FPDF_WIDESTRING msg, FPDF_WIDESTRING,
                    int, int) {
  // Deal with differences between UTF16LE and wchar_t on this platform.
  size_t characters = 0;
  while (msg[characters]) {
    ++characters;
  }
  wchar_t* platform_string =
      (wchar_t*)malloc((characters + 1) * sizeof(wchar_t));
  for (size_t i = 0; i < characters + 1; ++i) {
    unsigned char* ptr = (unsigned char*)&msg[i];
    platform_string[i] = ptr[0] + 256 * ptr[1];
  }
  printf("Alert: %ls\n", platform_string);
  free(platform_string);
  return 0;
}

void ExampleDocGotoPage(IPDF_JSPLATFORM*, int pageNumber) {
  printf("Goto Page: %d\n", pageNumber);
}

class TestLoader {
 public:
  TestLoader(const char* pBuf, size_t len);

  const char* m_pBuf;
  size_t m_Len;
};

TestLoader::TestLoader(const char* pBuf, size_t len)
    : m_pBuf(pBuf), m_Len(len) {
}

int Get_Block(void* param, unsigned long pos, unsigned char* pBuf,
              unsigned long size) {
  TestLoader* pLoader = (TestLoader*) param;
  if (pos + size < pos || pos + size > pLoader->m_Len) return 0;
  memcpy(pBuf, pLoader->m_pBuf + pos, size);
  return 1;
}

bool Is_Data_Avail(FX_FILEAVAIL* pThis, size_t offset, size_t size) {
  return true;
}

void Add_Segment(FX_DOWNLOADHINTS* pThis, size_t offset, size_t size) {
}

bool CheckDimensions(int iStride, int iWidth, int iHeight) {
  if (iStride < 0 || iWidth < 0 || iHeight < 0)
    return false;
  if (iHeight > 0 && iWidth > INT_MAX / iHeight)
    return false;
  return true;
}

#if _WIN32

void renderPdfToBMP(
    FPDF_PAGE iPage,
    const char* iData,
    const int iPageNum,
    const int iStride,
    const int iWidth,
    const int iHeight,
    std::string oError,
    std::string& oData) {
  if (iStride < 0 || iWidth < 0 || iHeight < 0) {
    return;
  }

  if (iHeight > 0 && iWidth > INT_MAX / iHeight) {
    return;
  }

  int out_len = iStride * iHeight;
  if (out_len > INT_MAX / 3) {
    return;
  }

  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(bmi) - sizeof(RGBQUAD);
  bmi.bmiHeader.biWidth = iWidth;
  bmi.bmiHeader.biHeight = -iHeight;  // top-down image
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;

  BITMAPFILEHEADER file_header = {0};
  file_header.bfType = 0x4d42;
  file_header.bfSize = sizeof(file_header) + bmi.bmiHeader.biSize + out_len;
  file_header.bfOffBits = file_header.bfSize - out_len;

  oData.clear();
  oData.append((const char*)(&file_header), sizeof(file_header));
  oData.append((const char*)(&bmi), bmi.bmiHeader.biSize);
  oData.append(iData, out_len);
}

void renderPdfToEMF(
    FPDF_PAGE iPage,
    const char* iData,
    const int iPageNum,
    const int iStride,
    const int iWidth,
    const int iHeight,
    std::string oError,
    std::string& oData) {
  int width = static_cast<int>(FPDF_GetPageWidth(iPage));
  int height = static_cast<int>(FPDF_GetPageHeight(iPage));

  HDC dc = CreateEnhMetaFileW(NULL, /*fileName=*/NULL, NULL, NULL);
  if (dc == NULL) {
    std::ostringstream error_str; // error string
    error_str << "error in page " << iPageNum << " to CreateEnhMetaFileW";
    oError = error_str.str();
    return;
  }

  HRGN rgn = CreateRectRgn(0, 0, width, height);
  SelectClipRgn(dc, rgn);
  DeleteObject(rgn);

  SelectObject(dc, GetStockObject(NULL_PEN));
  SelectObject(dc, GetStockObject(WHITE_BRUSH));
  // If a PS_NULL pen is used, the dimensions of the rectangle are 1 pixel less.
  Rectangle(dc, 0, 0, width + 1, height + 1);

  FPDF_RenderPage(dc, iPage, 0, 0, width, height, 0,
                  FPDF_ANNOT | FPDF_PRINTING | FPDF_NO_CATCH);

  // Get result
  HENHMETAFILE emf = CloseEnhMetaFile(dc);
  // TODO: Save it
  if(emf != NULL) {
    UINT emf_size = GetEnhMetaFileBits(emf, 0, NULL);
    oData.clear();
    oData.resize(emf_size);

    UINT size2 = GetEnhMetaFileBits(emf, emf_size, reinterpret_cast<BYTE*>((void*)(oData.c_str())));
    if(emf_size != size2) {
      oError = "error on read data from EMF: different size";
    }

    DeleteEnhMetaFile(emf);
  } else {
    oError = "error to get result EMF data";
  }
}

#endif  // _WIN32

void renderPdfToPNG(
    FPDF_PAGE iPage,
    const char* iData,
    const int iPageNum,
    const int iStride,
    const int iWidth,
    const int iHeight,
    std::string oError,
    std::string& oData) {
  if (!CheckDimensions(iStride, iWidth, iHeight))
    return;

  std::vector<unsigned char> png_encoding;
  const unsigned char* buffer = static_cast<const unsigned char*>((void*)iData);
  if (!image_diff_png::EncodeBGRAPNG(buffer, iWidth, iHeight, iStride, false, &png_encoding)) {
    std::ostringstream error_str; // error string
    error_str << "Error in page " << iPageNum << ": Failed to convert bitmap to PNG.";
    oError += error_str.str();
    return;
  }

  oData.clear();
  oData.resize(png_encoding.size());
  int i = 0;
  for(std::vector<unsigned char>::const_iterator it = png_encoding.begin(); it != png_encoding.end(); ++it) {
    oData[i++] = *it;
  }
}

void renderPdfToPPM(
    FPDF_PAGE iPage,
    const char* iData,
    const int iPageNum,
    const int iStride,
    const int iWidth,
    const int iHeight,
    std::string oError,
    std::string& oData) {
  if (!CheckDimensions(iStride, iWidth, iHeight))
    return;

  int out_len = iWidth * iHeight;
  if (out_len > INT_MAX / 3)
    return;
  out_len *= 3;

  std::ostringstream result_str;
  result_str << "P6\n# PDF test render\n" << iWidth << " " << iHeight << "\n255\n";
  oData = result_str.str();
  // Source data is B, G, R, unused.
  // Dest data is R, G, B.
  char* result = new char[out_len];
  if (result) {
    for (int h = 0; h < iHeight; ++h) {
      const char* src_line = iData + (iStride * h);
      char* dest_line = result + (iWidth * h * 3);
      for (int w = 0; w < iWidth; ++w) {
        // R
        dest_line[w * 3] = src_line[(w * 4) + 2];
        // G
        dest_line[(w * 3) + 1] = src_line[(w * 4) + 1];
        // B
        dest_line[(w * 3) + 2] = src_line[w * 4];
      }
    }

    oData.append(result, out_len);
    delete [] result;
  }
}

// Initialization and destroy module
class PdfModule {
public:
  typedef void (*FormatCallbackType)(FPDF_PAGE,
    const char*,
    const int,
    const int,
    const int,
    const int,
    std::string,
    std::string&);
  typedef std::map<std::string, FormatCallbackType> RenderFormatsType;
  /// This method require to have only 1 copy of PDF module
  static PdfModule& GetModule() {
    static PdfModule module;
    return module;
  }

  PdfModule() {
    FPDF_InitLibrary();

    memset(&unsupported_info, '\0', sizeof(unsupported_info));
    unsupported_info.version = 1;
    unsupported_info.FSDK_UnSupport_Handler = ExampleUnsupportedHandler;

    FSDK_SetUnSpObjProcessHandler(&unsupported_info);

#if _WIN32
    outputFormats.insert(std::make_pair("BMP", renderPdfToBMP));
    outputFormats.insert(std::make_pair("EMF", renderPdfToEMF));
#endif  // _WIN32
    outputFormats.insert(std::make_pair("PNG", renderPdfToPNG));
    outputFormats.insert(std::make_pair("PPM", renderPdfToPPM));

    uv_mutex_init(&PdfProcessingMutex);
  }

  ~PdfModule() {
    ShutdownSDK();
  }

  const RenderFormatsType& getOutputFormats() {
    return outputFormats;
  }

  struct Lock {
    Lock() {
      uv_mutex_lock(&(PdfModule::GetModule().PdfProcessingMutex));
    }

    ~Lock() {
      uv_mutex_unlock(&(PdfModule::GetModule().PdfProcessingMutex));
    }
  };

private:
  void ShutdownSDK() {
    FPDF_DestroyLibrary();
    uv_mutex_destroy(&PdfProcessingMutex);
  }

  UNSUPPORT_INFO unsupported_info;
  RenderFormatsType outputFormats;
  uv_mutex_t PdfProcessingMutex;
};

struct RenderAsyncReq {
  uv_work_t req;
  // input
  std::string data;
  std::string outputFormat;
  double scaleFactor;

  // output
  std::string error;
  std::vector<std::string> result;

  // callback
  v8::Persistent<v8::Function> callback;

  RenderAsyncReq() : scaleFactor(1.0) {
    req.data = this;
  };
};

void RenderAsync(uv_work_t *r) {
  // TODO (ionlupascu@gmail.com): resolve raise condition and remove mutex:
  // node(34942,0x103c0a000) malloc: *** error for object 0x103906eb8: incorrect checksum for freed object - object was probably modified after being freed.
  PdfModule::Lock processingLock;
  RenderAsyncReq* req = reinterpret_cast<RenderAsyncReq*>(r->data);
  PdfModule::RenderFormatsType::const_iterator renderRoutineCbIt = PdfModule::GetModule().getOutputFormats().find(req->outputFormat);
  if(renderRoutineCbIt == PdfModule::GetModule().getOutputFormats().end()) {
    req->error = "Invalid outputFormat. Use one from getSupportedOutputFormats";
    return;
  }

  PdfModule::FormatCallbackType renderRoutineCb = renderRoutineCbIt->second;

  IPDF_JSPLATFORM platform_callbacks;
  memset(&platform_callbacks, '\0', sizeof(platform_callbacks));
  platform_callbacks.version = 1;
  platform_callbacks.app_alert = ExampleAppAlert;
  platform_callbacks.Doc_gotoPage = ExampleDocGotoPage;

  FPDF_FORMFILLINFO form_callbacks;
  memset(&form_callbacks, '\0', sizeof(form_callbacks));
  form_callbacks.version = 1;
  form_callbacks.m_pJsPlatform = &platform_callbacks;

  TestLoader loader(req->data.c_str(), req->data.size());

  FPDF_FILEACCESS file_access;
  memset(&file_access, '\0', sizeof(file_access));
  file_access.m_FileLen = static_cast<unsigned long>(req->data.size());
  file_access.m_GetBlock = Get_Block;
  file_access.m_Param = &loader;

  FX_FILEAVAIL file_avail;
  memset(&file_avail, '\0', sizeof(file_avail));
  file_avail.version = 1;
  file_avail.IsDataAvail = Is_Data_Avail;

  FX_DOWNLOADHINTS hints;
  memset(&hints, '\0', sizeof(hints));
  hints.version = 1;
  hints.AddSegment = Add_Segment;

  FPDF_DOCUMENT doc;
  FPDF_AVAIL pdf_avail = FPDFAvail_Create(&file_avail, &file_access);

  (void) FPDFAvail_IsDocAvail(pdf_avail, &hints);

  if (!FPDFAvail_IsLinearized(pdf_avail)) {
    fprintf(stderr, "Non-linearized path...\n");
    doc = FPDF_LoadCustomDocument(&file_access, NULL);
  } else {
    fprintf(stderr, "Linearized path...\n");
    doc = FPDFAvail_GetDocument(pdf_avail, NULL);
  }

  (void) FPDF_GetDocPermissions(doc);
  (void) FPDFAvail_IsFormAvail(pdf_avail, &hints);

  FPDF_FORMHANDLE form = FPDFDOC_InitFormFillEnvironment(doc, &form_callbacks);
  // TODO (ionlupascu@gmail.com): to find out these magic numbers
  FPDF_SetFormFieldHighlightColor(form, 0, 0xFFE4DD);
  FPDF_SetFormFieldHighlightAlpha(form, 100);

  int first_page = FPDFAvail_GetFirstPageNum(doc);
  (void) FPDFAvail_IsPageAvail(pdf_avail, first_page, &hints);

  int page_count = FPDF_GetPageCount(doc);
  for (int i = 0; i < page_count; ++i) {
    (void) FPDFAvail_IsPageAvail(pdf_avail, i, &hints);
  }

  FORM_DoDocumentJSAction(form);
  FORM_DoDocumentOpenAction(form);

  size_t rendered_pages = 0;
  size_t bad_pages = 0;
  req->result.clear();
  req->result.resize(page_count);
  for (int i = 0; i < page_count; ++i) {
    FPDF_PAGE page = FPDF_LoadPage(doc, i);
    if (!page) {
        ++bad_pages;
        continue;
    }
    FPDF_TEXTPAGE text_page = FPDFText_LoadPage(page);
    FORM_OnAfterLoadPage(page, form);
    FORM_DoPageAAction(page, form, FPDFPAGE_AACTION_OPEN);

    double scale = req->scaleFactor;
    int width = static_cast<int>(FPDF_GetPageWidth(page) * scale);
    int height = static_cast<int>(FPDF_GetPageHeight(page) * scale);

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);
    ++rendered_pages;

    FPDF_FFLDraw(form, bitmap, page, 0, 0, width, height, 0, 0);
    int stride = FPDFBitmap_GetStride(bitmap);
    const char* buffer =
        reinterpret_cast<const char*>(FPDFBitmap_GetBuffer(bitmap));
    renderRoutineCb(page, buffer, i, stride, width, height, req->error, req->result[i]);

    FPDFBitmap_Destroy(bitmap);

    FORM_DoPageAAction(page, form, FPDFPAGE_AACTION_CLOSE);
    FORM_OnBeforeClosePage(page, form);
    FPDFText_ClosePage(text_page);
    FPDF_ClosePage(page);

    if(!req->error.empty()) {
      break;
    }
  }

  FORM_DoDocumentAAction(form, FPDFDOC_AACTION_WC);
  FPDFDOC_ExitFormFillEnvironment(form);
  FPDF_CloseDocument(doc);
  FPDFAvail_Destroy(pdf_avail);

  //ConvertSync(req->data, req->outputFormat, req->error, req->result);
  //req->error = "Not implemented yet";
}

void EncodePagesResult(const std::vector<std::string>& iResult, v8::Handle<v8::Array> oResult) {
  MY_NODE_MODULE_ISOLATE_DECL;
  int i = 0;
  for(std::vector<std::string>::const_iterator it = iResult.begin(); it != iResult.end(); ++it) {
    v8::MaybeLocal<v8::Object> objTemp = node::Buffer::New(MY_NODE_MODULE_ISOLATE_PRE (char*)it->c_str(), it->size());
	v8::Local<v8::Value> data(objTemp.ToLocalChecked());
	
    oResult->Set(i++, data);
  }
}

void RenderAsyncAfter(uv_work_t *r) {
  MY_NODE_MODULE_HANDLESCOPE;
  RenderAsyncReq* req = reinterpret_cast<RenderAsyncReq*>(r->data);

  v8::TryCatch try_catch;

  v8::Local<v8::Function> callback = v8::Local<v8::Function>::New(MY_NODE_MODULE_ISOLATE_PRE req->callback);
  if(!req->error.empty()) {
    v8::Handle<v8::Value> argv[1] = {v8::Exception::TypeError(V8_STRING_NEW_UTF8(req->error.c_str()))};

    callback->Call(MY_NODE_MODULE_CONTEXT, 1, argv);
  } else {
    v8::Local<v8::Array> result = V8_VALUE_NEW_DEFAULT_V_0_11_10(Array);
    EncodePagesResult(req->result, result);

    v8::Handle<v8::Value> argv[2] = {
      v8::Null(MY_NODE_MODULE_ISOLATE),
      result
    };

    callback->Call(MY_NODE_MODULE_CONTEXT, 2, argv);
  }

  // cleanup
  req->callback.Reset();
  delete req;

  if (try_catch.HasCaught()) {
    node::FatalException(try_catch);
  }
}

}  // namespace

MY_NODE_MODULE_CALLBACK(getSupportedOutputFormats)
{
  MY_NODE_MODULE_HANDLESCOPE;
  v8::Local<v8::Array> result = V8_VALUE_NEW_DEFAULT_V_0_11_10(Array);
  int i = 0;
  const PdfModule::RenderFormatsType& formats = PdfModule::GetModule().getOutputFormats();
  for(PdfModule::RenderFormatsType::const_iterator itFormat = formats.begin(); itFormat != formats.end(); ++itFormat)
  {
    result->Set(i++, V8_STRING_NEW_UTF8(itFormat->first.c_str()));
  }
  MY_NODE_MODULE_RETURN_VALUE(result);
}

MY_NODE_MODULE_CALLBACK(render)
{
  MY_NODE_MODULE_HANDLESCOPE;
  REQUIRE_ARGUMENT_OBJECT(iArgs, 0, options);
  OPTIONAL_ARGUMENT_FUNCTION(iArgs, 1, callback);

  if(!options->HasOwnProperty(V8_STRING_NEW_UTF8("data"))) {
    RETURN_EXCEPTION_STR_CB("data field is missing", callback);
  }

  if(!options->HasOwnProperty(V8_STRING_NEW_UTF8("outputFormat"))) {
    RETURN_EXCEPTION_STR_CB("outputFormat field is missing", callback);
  }

  MemValueBase<RenderAsyncReq> req;
  req.set(new RenderAsyncReq());
  v8::String::Utf8Value outputFormatObject(options->Get(V8_STRING_NEW_UTF8("outputFormat"))->ToString());
  v8::Local<v8::Object> dataobject = options->Get(V8_STRING_NEW_UTF8("data")).As<v8::Object>();

  if(dataobject->IsObject() && dataobject->IsUint8Array())
  {
    req->data.assign(static_cast<char*>(dataobject.As<v8::Uint16Array>()->Buffer()->GetContents().Data()),
            dataobject.As<v8::Uint16Array>()->Length());
  }
  else
  {
    RETURN_EXCEPTION_STR_CB("data must be a Buffer", callback);
  }

  req->outputFormat.assign(*outputFormatObject, outputFormatObject.length());

  if(options->HasOwnProperty(V8_STRING_NEW_UTF8("scaleFactor"))) {
    v8::Local<v8::Value> scaleFactor = options->Get(V8_STRING_NEW_UTF8("scaleFactor")).As<v8::Value>();
    if(!scaleFactor->IsNumber()) {
      RETURN_EXCEPTION_STR_CB("scaleFactor should be a Number", callback);
    }

    req->scaleFactor = scaleFactor->NumberValue();
  }

  if(!callback.IsEmpty()) {
    // async call
    req->callback.Reset(MY_NODE_MODULE_ISOLATE_PRE callback);
    uv_queue_work(uv_default_loop(), &(req.release()->req), RenderAsync, (uv_after_work_cb)RenderAsyncAfter);
    MY_NODE_MODULE_RETURN_VALUE(v8::Undefined(MY_NODE_MODULE_ISOLATE));
  } else {
    // sync call
    RenderAsync(&(req->req));
    if(!req->error.empty()) {
      RETURN_EXCEPTION_STR(req->error.c_str());
    } else {
      v8::Local<v8::Array> result = V8_VALUE_NEW_DEFAULT_V_0_11_10(Array);
      EncodePagesResult(req->result, result);
      MY_NODE_MODULE_RETURN_VALUE(result);
    }
  }
}

void initNode(v8::Handle<v8::Object> exports) {
  // Init library
  PdfModule::GetModule();

  // only for node
  NODE_SET_METHOD(exports, "render", render);
  NODE_SET_METHOD(exports, "getSupportedOutputFormats", getSupportedOutputFormats);
}

NODE_MODULE(node_printer, initNode);
