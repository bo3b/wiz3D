/*
 * Minimal LlamaXML implementation for iZ3D build.
 * Implements the subset of LlamaXML used by CommandDumper (XMLWriter)
 * and DX10SharedLibrary (XMLReader).
 * Based on the original LlamaXML headers (GPL v2, Llamagraphics Inc.)
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <string>
#include <algorithm>

#include "LlamaXML/OutputStream.h"
#include "LlamaXML/FileOutputStream.h"
#include "LlamaXML/TextEncoding.h"
#include "LlamaXML/ConvertToUnicode.h"
#include "LlamaXML/ConvertFromUnicode.h"
#include "LlamaXML/XMLWriter.h"
#include "LlamaXML/XMLReader.h"
#include "LlamaXML/XMLException.h"
#include "LlamaXML/InputStream.h"
#include "LlamaXML/BufferInputStream.h"
#include "LlamaXML/FileInputStream.h"
#include "LlamaXML/StringInputStream.h"

namespace LlamaXML {

// ---- OutputStream ----
OutputStream::~OutputStream() {}

// ---- FileOutputStream ----
FileOutputStream::FileOutputStream(const char * path)
    : mFile(INVALID_HANDLE_VALUE)
{
    mFile = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);
}

FileOutputStream::FileOutputStream(const wchar_t * path)
    : mFile(INVALID_HANDLE_VALUE)
{
    mFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);
}

FileOutputStream::~FileOutputStream()
{
    if (mFile != INVALID_HANDLE_VALUE)
        CloseHandle(mFile);
}

void FileOutputStream::WriteData(const char * buffer, uint32_t length)
{
    if (mFile != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(mFile, buffer, length, &written, NULL);
    }
}

// ---- TextEncoding ----
TextEncoding::TextEncoding()
    : mCodePage(CP_ACP)
{
}

bool TextEncoding::IsAvailable() const
{
    return true;
}

TextEncoding TextEncoding::System()
{
    return TextEncoding(GetACP());
}

TextEncoding TextEncoding::Application()
{
    return TextEncoding(GetACP());
}

TextEncoding TextEncoding::WindowsLatin1()  { return TextEncoding(1252); }
TextEncoding TextEncoding::PalmLatin1()     { return TextEncoding(1252); }
TextEncoding TextEncoding::ISOLatin1()      { return TextEncoding(28591); }
TextEncoding TextEncoding::ASCII()          { return TextEncoding(20127); }
TextEncoding TextEncoding::WindowsShiftJIS(){ return TextEncoding(932); }
TextEncoding TextEncoding::PalmShiftJIS()   { return TextEncoding(932); }
TextEncoding TextEncoding::ShiftJIS()       { return TextEncoding(932); }
TextEncoding TextEncoding::UCS2()           { return TextEncoding(1200); }
TextEncoding TextEncoding::UTF7()           { return TextEncoding(CP_UTF7); }
TextEncoding TextEncoding::UTF8()           { return TextEncoding(CP_UTF8); }
TextEncoding TextEncoding::UTF16()          { return TextEncoding(1200); }
TextEncoding TextEncoding::UTF16BE()        { return TextEncoding(1201); }
TextEncoding TextEncoding::UTF16LE()        { return TextEncoding(1200); }
TextEncoding TextEncoding::UTF32()          { return TextEncoding(12000); }
TextEncoding TextEncoding::UTF32BE()        { return TextEncoding(12001); }
TextEncoding TextEncoding::UTF32LE()        { return TextEncoding(12000); }

TextEncoding TextEncoding::Windows(UINT codePage)
{
    return TextEncoding(codePage);
}

TextEncoding TextEncoding::WebCharset(const UnicodeChar * /*name*/)
{
    return TextEncoding(CP_UTF8);
}

// ---- ConvertToUnicode ----
ConvertToUnicode::ConvertToUnicode(TextEncoding sourceEncoding)
    : mSourceEncoding(sourceEncoding)
{
}

ConvertToUnicode::~ConvertToUnicode()
{
}

void ConvertToUnicode::Reset(TextEncoding sourceEncoding)
{
    mSourceEncoding = sourceEncoding;
}

void ConvertToUnicode::Convert(const char * & sourceStart, const char * sourceEnd,
    UnicodeChar * & destStart, UnicodeChar * destEnd)
{
    int srcLen = (int)(sourceEnd - sourceStart);
    int destLen = (int)(destEnd - destStart);
    if (srcLen <= 0 || destLen <= 0) return;

    int result = MultiByteToWideChar(mSourceEncoding.AsWindowsCodePage(), 0,
        sourceStart, srcLen, destStart, destLen);
    if (result > 0) {
        sourceStart = sourceEnd;
        destStart += result;
    }
}

// ---- ConvertFromUnicode ----
ConvertFromUnicode::ConvertFromUnicode(TextEncoding destinationEncoding)
    : mDestinationEncoding(destinationEncoding)
{
}

ConvertFromUnicode::~ConvertFromUnicode()
{
}

void ConvertFromUnicode::Convert(const UnicodeChar * & sourceStart, const UnicodeChar * sourceEnd,
    char * & destStart, char * destEnd)
{
    int srcLen = (int)(sourceEnd - sourceStart);
    int destLen = (int)(destEnd - destStart);
    if (srcLen <= 0 || destLen <= 0) return;

    int result = WideCharToMultiByte(mDestinationEncoding.AsWindowsCodePage(), 0,
        sourceStart, srcLen, destStart, destLen, NULL, NULL);
    if (result > 0) {
        sourceStart = sourceEnd;
        destStart += result;
    }
}

// ---- XMLException ----
XMLException::XMLException(int32_t err, const char * file, long line) throw()
    : mErrorCode(err), mFile(file), mLine(line), mWhat("XMLException")
{
}

XMLException::XMLException(int32_t err, const char * what) throw()
    : mErrorCode(err), mFile(""), mLine(0), mWhat(what ? what : "XMLException")
{
}

const char * XMLException::what() const throw()
{
    return mWhat;
}

void ThrowXMLException(int32_t err, const char * file, long line)
{
    throw XMLException(err, file, line);
}

void ThrowXMLException(int32_t err, const char * what)
{
    throw XMLException(err, what);
}

// ---- XMLWriter ----
const char * XMLWriter::kNewline    = "\n";
const char * XMLWriter::kIndent     = "  ";
const char * XMLWriter::kAmpersand  = "&amp;";
const char * XMLWriter::kLessThan   = "&lt;";
const char * XMLWriter::kGreaterThan= "&gt;";
const char * XMLWriter::kQuote      = "&quot;";

XMLWriter::XMLWriter(OutputStream & output, TextEncoding applicationEncoding)
    : mOutput(output)
    , mState(kStateNormal)
    , mIndentLevel(0)
    , mApplicationToUnicode(applicationEncoding)
    , mUnicodeToUTF8(TextEncoding::UTF8())
{
}

XMLWriter::~XMLWriter()
{
}

void XMLWriter::StartDocument(const char * version, const char * encoding, const char * standalone)
{
    std::string decl = "<?xml";
    if (version) {
        decl += " version=\"";
        decl += version;
        decl += "\"";
    }
    if (encoding) {
        decl += " encoding=\"";
        decl += encoding;
        decl += "\"";
    } else {
        decl += " encoding=\"UTF-8\"";
    }
    if (standalone) {
        decl += " standalone=\"";
        decl += standalone;
        decl += "\"";
    }
    decl += "?>";
    decl += kNewline;
    mOutput.WriteData(decl.c_str(), (uint32_t)decl.size());
    mState = kStateNormal;
}

void XMLWriter::EndDocument()
{
    mState = kStateDocumentClosed;
}

const char * XMLWriter::Scan(const char * content, const char * tokens)
{
    while (*content) {
        for (const char * t = tokens; *t; ++t) {
            if (*content == *t) return content;
        }
        ++content;
    }
    return content;
}

const UnicodeChar * XMLWriter::Scan(const UnicodeChar * content, const UnicodeChar * contentEnd,
    const char * tokens)
{
    while (content < contentEnd) {
        for (const char * t = tokens; *t; ++t) {
            if (*content == (UnicodeChar)*t) return content;
        }
        ++content;
    }
    return content;
}

const char * XMLWriter::StringEnd(const char * s)
{
    return s + strlen(s);
}

const UnicodeChar * XMLWriter::StringEnd(const UnicodeChar * s)
{
    const UnicodeChar * p = s;
    while (*p) ++p;
    return p;
}

void XMLWriter::WriteRawUnicode(const UnicodeChar * unicodeStart, const UnicodeChar * unicodeEnd)
{
    char buf[1024];
    while (unicodeStart < unicodeEnd) {
        char * destStart = buf;
        char * destEnd = buf + sizeof(buf);
        mUnicodeToUTF8.Convert(unicodeStart, unicodeEnd, destStart, destEnd);
        if (destStart > buf) {
            mOutput.WriteData(buf, (uint32_t)(destStart - buf));
        }
    }
}

void XMLWriter::WriteApplicationContent(const char * content)
{
    // Convert from application encoding to UTF-8 via Unicode
    const char * srcEnd = StringEnd(content);
    while (content < srcEnd) {
        UnicodeChar unicodeBuf[512];
        UnicodeChar * uniDest = unicodeBuf;
        UnicodeChar * uniEnd = unicodeBuf + 512;
        mApplicationToUnicode.Convert(content, srcEnd, uniDest, uniEnd);
        WriteUnicodeContent(unicodeBuf, uniDest);
    }
}

void XMLWriter::WriteUTF8Content(const char * content)
{
    // Escape XML special chars
    while (*content) {
        const char * safe = Scan(content, "<>&\"");
        if (safe > content) {
            mOutput.WriteData(content, (uint32_t)(safe - content));
        }
        if (*safe == '<') {
            mOutput.WriteData(kLessThan, (uint32_t)strlen(kLessThan));
            ++safe;
        } else if (*safe == '>') {
            mOutput.WriteData(kGreaterThan, (uint32_t)strlen(kGreaterThan));
            ++safe;
        } else if (*safe == '&') {
            mOutput.WriteData(kAmpersand, (uint32_t)strlen(kAmpersand));
            ++safe;
        } else if (*safe == '"') {
            mOutput.WriteData(kQuote, (uint32_t)strlen(kQuote));
            ++safe;
        }
        content = safe;
    }
}

void XMLWriter::WriteUnicodeContent(const UnicodeChar * content, const UnicodeChar * contentEnd)
{
    // Convert to UTF-8 and write with escaping
    char buf[1024];
    while (content < contentEnd) {
        const UnicodeChar * safe = Scan(content, contentEnd, "<>&\"");
        if (safe > content) {
            const UnicodeChar * src = content;
            while (src < safe) {
                char * dest = buf;
                char * dEnd = buf + sizeof(buf);
                mUnicodeToUTF8.Convert(src, safe, dest, dEnd);
                if (dest > buf) {
                    mOutput.WriteData(buf, (uint32_t)(dest - buf));
                }
            }
        }
        if (safe < contentEnd) {
            if (*safe == '<') {
                mOutput.WriteData(kLessThan, (uint32_t)strlen(kLessThan));
            } else if (*safe == '>') {
                mOutput.WriteData(kGreaterThan, (uint32_t)strlen(kGreaterThan));
            } else if (*safe == '&') {
                mOutput.WriteData(kAmpersand, (uint32_t)strlen(kAmpersand));
            } else if (*safe == '"') {
                mOutput.WriteData(kQuote, (uint32_t)strlen(kQuote));
            }
            ++safe;
        }
        content = safe;
    }
}

void XMLWriter::StartElement(const char * name)
{
    if (mState == kStateOpenTag) {
        mOutput.WriteData(">", 1);
        mOutput.WriteData(kNewline, (uint32_t)strlen(kNewline));
    }
    for (size_t i = 0; i < mIndentLevel; ++i) {
        mOutput.WriteData(kIndent, (uint32_t)strlen(kIndent));
    }
    mOutput.WriteData("<", 1);
    mOutput.WriteData(name, (uint32_t)strlen(name));
    mElementStack.push_back(name);
    mIndentLevel++;
    mState = kStateOpenTag;
}

void XMLWriter::StartElement(const char * prefix, const char * name, const char * namespaceURI)
{
    if (prefix && *prefix) {
        std::string fullName = std::string(prefix) + ":" + name;
        StartElement(fullName.c_str());
    } else {
        StartElement(name);
    }
    if (namespaceURI && *namespaceURI) {
        std::string attrName = std::string("xmlns");
        if (prefix && *prefix) {
            attrName += ":";
            attrName += prefix;
        }
        WriteAttribute(attrName.c_str(), std::string(namespaceURI));
    }
}

void XMLWriter::EndElement()
{
    mIndentLevel--;
    if (mState == kStateOpenTag) {
        mOutput.WriteData("/>", 2);
        mOutput.WriteData(kNewline, (uint32_t)strlen(kNewline));
    } else {
        for (size_t i = 0; i < mIndentLevel; ++i) {
            mOutput.WriteData(kIndent, (uint32_t)strlen(kIndent));
        }
        mOutput.WriteData("</", 2);
        if (!mElementStack.empty()) {
            const std::string & tag = mElementStack.back();
            mOutput.WriteData(tag.c_str(), (uint32_t)tag.size());
        }
        mOutput.WriteData(">", 1);
        mOutput.WriteData(kNewline, (uint32_t)strlen(kNewline));
    }
    if (!mElementStack.empty())
        mElementStack.pop_back();
    mState = kStateNormal;
}

void XMLWriter::WriteElement(const char * name)
{
    StartElement(name);
    EndElement();
}

void XMLWriter::WriteComment(const char * content)
{
    if (mState == kStateOpenTag) {
        mOutput.WriteData(">", 1);
        mOutput.WriteData(kNewline, (uint32_t)strlen(kNewline));
        mState = kStateNormal;
    }
    mOutput.WriteData("<!-- ", 5);
    mOutput.WriteData(content, (uint32_t)strlen(content));
    mOutput.WriteData(" -->", 4);
    mOutput.WriteData(kNewline, (uint32_t)strlen(kNewline));
}

void XMLWriter::WriteComment(const UnicodeChar * content)
{
    if (mState == kStateOpenTag) {
        mOutput.WriteData(">", 1);
        mOutput.WriteData(kNewline, (uint32_t)strlen(kNewline));
        mState = kStateNormal;
    }
    mOutput.WriteData("<!-- ", 5);
    WriteRawUnicode(content, StringEnd(content));
    mOutput.WriteData(" -->", 4);
    mOutput.WriteData(kNewline, (uint32_t)strlen(kNewline));
}

void XMLWriter::WriteComment(const UnicodeString & content)
{
    WriteComment(content.c_str());
}

void XMLWriter::WriteString(const char * content)
{
    if (mState == kStateOpenTag) {
        mOutput.WriteData(">", 1);
        mState = kStateNormal;
    }
    WriteUTF8Content(content);
}

void XMLWriter::WriteString(const UnicodeChar * content)
{
    if (mState == kStateOpenTag) {
        mOutput.WriteData(">", 1);
        mState = kStateNormal;
    }
    WriteUnicodeContent(content, StringEnd(content));
}

void XMLWriter::WriteString(const UnicodeString & content)
{
    WriteString(content.c_str());
}

void XMLWriter::StartAttribute(const char * name)
{
    mOutput.WriteData(" ", 1);
    mOutput.WriteData(name, (uint32_t)strlen(name));
    mOutput.WriteData("=\"", 2);
    mState = kStateOpenAttribute;
}

void XMLWriter::StartAttribute(const char * prefix, const char * name, const char * /*namespaceURI*/)
{
    if (prefix && *prefix) {
        std::string fullName = std::string(prefix) + ":" + name;
        StartAttribute(fullName.c_str());
    } else {
        StartAttribute(name);
    }
}

void XMLWriter::EndAttribute()
{
    mOutput.WriteData("\"", 1);
    mState = kStateOpenTag;
}

void XMLWriter::StartComment()
{
    mOutput.WriteData("<!-- ", 5);
}

void XMLWriter::EndComment()
{
    mOutput.WriteData(" -->", 4);
}

// ---- InputStream ----
InputStream::~InputStream() {}

// ---- BufferInputStream ----
BufferInputStream::BufferInputStream(const void * buffer, uint32_t length)
    : mBuffer(static_cast<const char*>(buffer)), mLength(length), mOffset(0)
{
}

uint32_t BufferInputStream::ReadUpTo(char * buffer, uint32_t length)
{
    uint32_t available = mLength - mOffset;
    uint32_t toRead = (std::min)(length, available);
    if (toRead > 0) {
        memcpy(buffer, mBuffer + mOffset, toRead);
        mOffset += toRead;
    }
    return toRead;
}

void BufferInputStream::Restart()
{
    mOffset = 0;
}

bool BufferInputStream::EndOfFile()
{
    return mOffset >= mLength;
}

// ---- FileInputStream ----
FileInputStream::FileInputStream(const char * path)
    : mFile(INVALID_HANDLE_VALUE)
{
    mFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

FileInputStream::FileInputStream(const wchar_t * path)
    : mFile(INVALID_HANDLE_VALUE)
{
    mFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

FileInputStream::~FileInputStream()
{
    if (mFile != INVALID_HANDLE_VALUE)
        CloseHandle(mFile);
}

uint32_t FileInputStream::ReadUpTo(char * buffer, uint32_t length)
{
    if (mFile == INVALID_HANDLE_VALUE) return 0;
    DWORD bytesRead = 0;
    ReadFile(mFile, buffer, length, &bytesRead, NULL);
    return bytesRead;
}

void FileInputStream::Restart()
{
    if (mFile != INVALID_HANDLE_VALUE)
        SetFilePointer(mFile, 0, NULL, FILE_BEGIN);
}

bool FileInputStream::EndOfFile()
{
    if (mFile == INVALID_HANDLE_VALUE) return true;
    DWORD pos = SetFilePointer(mFile, 0, NULL, FILE_CURRENT);
    DWORD size = GetFileSize(mFile, NULL);
    return pos >= size;
}

// ---- StringInputStream ----
StringInputStream::StringInputStream(const std::string & s)
    : mString(s), mOffset(0)
{
}

StringInputStream::StringInputStream(const char * s)
    : mString(s ? s : ""), mOffset(0)
{
}

uint32_t StringInputStream::ReadUpTo(char * buffer, uint32_t length)
{
    uint32_t available = static_cast<uint32_t>(mString.size()) - mOffset;
    uint32_t toRead = (std::min)(length, available);
    if (toRead > 0) {
        memcpy(buffer, mString.data() + mOffset, toRead);
        mOffset += toRead;
    }
    return toRead;
}

void StringInputStream::Restart()
{
    mOffset = 0;
}

bool StringInputStream::EndOfFile()
{
    return mOffset >= static_cast<uint32_t>(mString.size());
}

// ---- XMLReader ----
// Static members
UnicodeString XMLReader::sEmptyUniCharString;
const XMLReader::UniCharRange XMLReader::sBaseCharRanges[] = { {0x0041, 0x005A}, {0x0061, 0x007A} };
const XMLReader::UniCharRange XMLReader::sCombiningCharRanges[] = { {0x0300, 0x0345} };
const XMLReader::UniCharRange XMLReader::sDigitRanges[] = { {0x0030, 0x0039} };

XMLReader::XMLReader(InputStream & input, TextEncoding initialEncoding)
    : mWhitespaceHandling(kWhitespaceHandlingAll)
    , mInput(input)
    , mDocumentEncoding(initialEncoding)
    , mInputBuffer(new char[kInputBufferCount])
    , mInputStart(mInputBuffer)
    , mInputEnd(mInputBuffer)
    , mOutputBuffer(new UnicodeChar[kOutputBufferCount])
    , mOutputStart(mOutputBuffer)
    , mOutputEnd(mOutputBuffer)
    , mNodeType(kNone)
    , mIsEmptyElement(false)
    , mConverter(initialEncoding)
    , mSkipContent(false)
{
}

XMLReader::~XMLReader()
{
    delete[] mInputBuffer;
    delete[] mOutputBuffer;
}

bool XMLReader::Read()
{
    return ReadInternal();
}

bool XMLReader::ReadInternal()
{
    // Minimal parsing: read from input, detect elements
    // Fill buffers if needed
    if (mOutputStart >= mOutputEnd) {
        FillOutputBuffer();
        if (mOutputStart >= mOutputEnd)
            return false; // EOF
    }

    // Skip whitespace
    while (mOutputStart < mOutputEnd && (*mOutputStart == ' ' || *mOutputStart == '\t' ||
           *mOutputStart == '\r' || *mOutputStart == '\n')) {
        ++mOutputStart;
        if (mOutputStart >= mOutputEnd) {
            FillOutputBuffer();
            if (mOutputStart >= mOutputEnd) return false;
        }
    }

    if (mOutputStart >= mOutputEnd) return false;

    if (*mOutputStart == '<') {
        ++mOutputStart;
        if (mOutputStart >= mOutputEnd) FillOutputBuffer();
        if (mOutputStart >= mOutputEnd) return false;

        if (*mOutputStart == '/') {
            // End element
            ++mOutputStart;
            mNodeType = kEndElement;
            // Read name until '>'
            mCurrentName.Clear();
            UnicodeString name;
            while (mOutputStart < mOutputEnd && *mOutputStart != '>') {
                name += *mOutputStart++;
                if (mOutputStart >= mOutputEnd) FillOutputBuffer();
            }
            if (mOutputStart < mOutputEnd) ++mOutputStart; // skip '>'
            mCurrentName.SetName(name);
            mCurrentName.DivideName();
            if (!mOpenTags.empty()) PopTag();
            return true;
        }
        else if (*mOutputStart == '!' || *mOutputStart == '?') {
            // Comment or processing instruction - skip to '>'
            mNodeType = kComment;
            while (mOutputStart < mOutputEnd) {
                if (*mOutputStart == '>') { ++mOutputStart; break; }
                ++mOutputStart;
                if (mOutputStart >= mOutputEnd) FillOutputBuffer();
            }
            return true;
        }
        else {
            // Start element
            mNodeType = kElement;
            mCurrentName.Clear();
            mAttributes.clear();
            UnicodeString name;
            // Read element name
            while (mOutputStart < mOutputEnd && *mOutputStart != '>' &&
                   *mOutputStart != ' ' && *mOutputStart != '/' &&
                   *mOutputStart != '\t' && *mOutputStart != '\r' && *mOutputStart != '\n') {
                name += *mOutputStart++;
                if (mOutputStart >= mOutputEnd) FillOutputBuffer();
            }
            mCurrentName.SetName(name);
            mCurrentName.DivideName();

            // Parse attributes
            while (mOutputStart < mOutputEnd) {
                // Skip whitespace
                while (mOutputStart < mOutputEnd && (*mOutputStart == ' ' || *mOutputStart == '\t' ||
                       *mOutputStart == '\r' || *mOutputStart == '\n')) {
                    ++mOutputStart;
                    if (mOutputStart >= mOutputEnd) FillOutputBuffer();
                }
                if (mOutputStart >= mOutputEnd) break;
                if (*mOutputStart == '>') { ++mOutputStart; mIsEmptyElement = false; break; }
                if (*mOutputStart == '/') {
                    ++mOutputStart;
                    if (mOutputStart >= mOutputEnd) FillOutputBuffer();
                    if (mOutputStart < mOutputEnd && *mOutputStart == '>') ++mOutputStart;
                    mIsEmptyElement = true;
                    break;
                }
                // Read attribute name
                Attribute attr;
                UnicodeString attrName;
                while (mOutputStart < mOutputEnd && *mOutputStart != '=' && *mOutputStart != ' ' &&
                       *mOutputStart != '>' && *mOutputStart != '/') {
                    attrName += *mOutputStart++;
                    if (mOutputStart >= mOutputEnd) FillOutputBuffer();
                }
                attr.SetName(attrName);
                attr.DivideName();
                // Skip '='
                if (mOutputStart < mOutputEnd && *mOutputStart == '=') ++mOutputStart;
                if (mOutputStart >= mOutputEnd) FillOutputBuffer();
                // Read attribute value (quoted)
                if (mOutputStart < mOutputEnd && (*mOutputStart == '"' || *mOutputStart == '\'')) {
                    UnicodeChar quote = *mOutputStart++;
                    if (mOutputStart >= mOutputEnd) FillOutputBuffer();
                    while (mOutputStart < mOutputEnd && *mOutputStart != quote) {
                        attr.mValue += *mOutputStart++;
                        if (mOutputStart >= mOutputEnd) FillOutputBuffer();
                    }
                    if (mOutputStart < mOutputEnd) ++mOutputStart; // skip closing quote
                }
                mAttributes.push_back(attr);
            }
            PushTag();
            return true;
        }
    }
    else {
        // Text node
        mNodeType = kText;
        mValue.clear();
        while (mOutputStart < mOutputEnd && *mOutputStart != '<') {
            mValue += *mOutputStart++;
            if (mOutputStart >= mOutputEnd) FillOutputBuffer();
        }
        return true;
    }
}

void XMLReader::FillInputBuffer()
{
    // Move any remaining data to the start
    size_t remaining = mInputEnd - mInputStart;
    if (remaining > 0 && mInputStart != mInputBuffer) {
        memmove(mInputBuffer, mInputStart, remaining);
    }
    mInputStart = mInputBuffer;
    mInputEnd = mInputBuffer + remaining;
    uint32_t bytesRead = mInput.ReadUpTo(mInputEnd,
        static_cast<uint32_t>(kInputBufferCount - remaining));
    mInputEnd += bytesRead;
}

void XMLReader::FillOutputBuffer()
{
    if (mInputStart >= mInputEnd) {
        FillInputBuffer();
        if (mInputStart >= mInputEnd) {
            mOutputStart = mOutputBuffer;
            mOutputEnd = mOutputBuffer;
            return;
        }
    }
    const char* srcStart = mInputStart;
    const char* srcEnd = mInputEnd;
    UnicodeChar* dstStart = mOutputBuffer;
    UnicodeChar* dstEnd = mOutputBuffer + kOutputBufferCount;
    mConverter.Convert(srcStart, srcEnd, dstStart, dstEnd);
    mInputStart = const_cast<char*>(srcStart);
    mOutputStart = mOutputBuffer;
    mOutputEnd = dstStart;
}

size_t XMLReader::ConvertInput(char * outputBuffer, size_t len)
{
    return 0;
}

bool XMLReader::BufferStartsWith(const char * prefix)
{
    return false;
}

bool XMLReader::StartsWithWhitespace()
{
    return mOutputStart < mOutputEnd && (*mOutputStart == ' ' || *mOutputStart == '\t' ||
           *mOutputStart == '\r' || *mOutputStart == '\n');
}

bool XMLReader::EndOfFile() const
{
    return mOutputStart >= mOutputEnd && mInput.EndOfFile();
}

XMLReader::NodeType XMLReader::MoveToContent()
{
    while (mNodeType != kElement && mNodeType != kEndElement && mNodeType != kText) {
        if (!Read()) return kNone;
    }
    return mNodeType;
}

bool XMLReader::IsStartElement()
{
    return MoveToContent() == kElement;
}

bool XMLReader::IsStartElement(const char * name)
{
    return IsStartElement() && Equals(GetLocalName(), name);
}

bool XMLReader::IsStartElement(const char * localName, const char * namespaceURI)
{
    return IsStartElement(localName);
}

void XMLReader::ReadStartElement()
{
    if (MoveToContent() != kElement)
        throw XMLException(0, "Expected start element");
    Read();
}

void XMLReader::ReadStartElement(const char * name)
{
    if (!IsStartElement(name))
        throw XMLException(0, "Expected start element");
    Read();
}

void XMLReader::ReadStartElement(const char * localName, const char * namespaceURI)
{
    ReadStartElement(localName);
}

bool XMLReader::IsNotEmptyElementRead()
{
    bool notEmpty = !mIsEmptyElement;
    Read();
    return notEmpty;
}

bool XMLReader::IsOpenElementRead()
{
    return IsNotEmptyElementRead();
}

bool XMLReader::IsOpenElementRead(const char * name)
{
    if (!IsStartElement(name)) return false;
    return IsNotEmptyElementRead();
}

bool XMLReader::IsOpenElementRead(const char * localName, const char * namespaceURI)
{
    return IsOpenElementRead(localName);
}

bool XMLReader::MoveToSubElement()
{
    while (true) {
        NodeType t = MoveToContent();
        if (t == kElement) return true;
        if (t == kEndElement) {
            Read(); // consume end element
            return false;
        }
        if (!Read()) return false;
    }
}

void XMLReader::ReadEndElement()
{
    if (MoveToContent() != kEndElement)
        throw XMLException(0, "Expected end element");
    Read();
}

void XMLReader::Skip()
{
    if (mNodeType == kElement && !mIsEmptyElement) {
        int depth = 1;
        while (depth > 0 && Read()) {
            if (mNodeType == kElement && !mIsEmptyElement) ++depth;
            else if (mNodeType == kEndElement) --depth;
        }
    }
    else {
        Read();
    }
}

UnicodeString XMLReader::ReadString()
{
    UnicodeString result;
    if (mNodeType == kElement) Read();
    while (mNodeType == kText || mNodeType == kWhitespace || mNodeType == kSignificantWhitespace) {
        result += mValue;
        if (!Read()) break;
    }
    return result;
}

std::string XMLReader::ReadString(TextEncoding encoding)
{
    UnicodeString us = ReadString();
    std::string result;
    for (size_t i = 0; i < us.size(); ++i) {
        result += static_cast<char>(us[i]);
    }
    return result;
}

UnicodeString XMLReader::ReadElementString()
{
    ReadStartElement();
    UnicodeString result = ReadString();
    return result;
}

UnicodeString XMLReader::ReadElementString(const char * name)
{
    ReadStartElement(name);
    return ReadString();
}

UnicodeString XMLReader::ReadElementString(const char * localName, const char * namespaceURI)
{
    return ReadElementString(localName);
}

std::string XMLReader::ReadElementString(TextEncoding encoding)
{
    ReadStartElement();
    return ReadString(encoding);
}

std::string XMLReader::ReadElementString(const char * name, TextEncoding encoding)
{
    ReadStartElement(name);
    return ReadString(encoding);
}

std::string XMLReader::ReadElementString(const char * localName, const char * namespaceURI,
    TextEncoding encoding)
{
    return ReadElementString(localName, encoding);
}

bool XMLReader::HasAttribute(size_t i) const
{
    return i < mAttributes.size();
}

bool XMLReader::HasAttribute(const char * name) const
{
    for (size_t i = 0; i < mAttributes.size(); ++i) {
        if (Equals(mAttributes[i].mLocalName, name)) return true;
    }
    return false;
}

bool XMLReader::HasAttribute(const char * localName, const char * namespaceURI) const
{
    return HasAttribute(localName);
}

UnicodeString XMLReader::GetAttribute(size_t i) const
{
    if (i < mAttributes.size()) return mAttributes[i].mValue;
    return sEmptyUniCharString;
}

UnicodeString XMLReader::GetAttribute(const char * name) const
{
    for (size_t i = 0; i < mAttributes.size(); ++i) {
        if (Equals(mAttributes[i].mLocalName, name)) return mAttributes[i].mValue;
    }
    return sEmptyUniCharString;
}

UnicodeString XMLReader::GetAttribute(const char * localName, const char * namespaceURI) const
{
    return GetAttribute(localName);
}

std::string XMLReader::GetAttribute(size_t i, TextEncoding encoding) const
{
    UnicodeString us = GetAttribute(i);
    std::string result;
    for (size_t j = 0; j < us.size(); ++j) result += static_cast<char>(us[j]);
    return result;
}

std::string XMLReader::GetAttribute(const char * name, TextEncoding encoding) const
{
    UnicodeString us = GetAttribute(name);
    std::string result;
    for (size_t j = 0; j < us.size(); ++j) result += static_cast<char>(us[j]);
    return result;
}

std::string XMLReader::GetAttribute(const char * localName, const char * namespaceURI,
    TextEncoding encoding) const
{
    return GetAttribute(localName, encoding);
}

// ---- XMLReader parsing helpers ----
bool XMLReader::ParseElement() { return false; }
bool XMLReader::ParseEndElement() { return false; }
bool XMLReader::ParseOptionalWhitespace() { return false; }
bool XMLReader::ParseRequiredWhitespace() { return false; }
bool XMLReader::ParseXmlDeclaration() { return false; }
bool XMLReader::ParseAttribute(Attribute &) { return false; }
bool XMLReader::ParseAttValue(UnicodeString &) { return false; }
bool XMLReader::ParseEq() { return false; }
bool XMLReader::ParseName(Name &) { return false; }
bool XMLReader::ParseReference(UnicodeString &) { return false; }
bool XMLReader::ParseString(const char *) { return false; }
bool XMLReader::ParseText() { return false; }
bool XMLReader::ParseComment() { return false; }
bool XMLReader::ParseCommentText() { return false; }
UnicodeChar XMLReader::PeekChar() { return 0; }
UnicodeChar XMLReader::ReadChar() { return 0; }
bool XMLReader::ParseChar(UnicodeChar) { return false; }

void XMLReader::LookupNamespace(Name &) const {}
size_t XMLReader::PushNamespaces() { return 0; }
void XMLReader::PopNamespaces(size_t) {}
void XMLReader::PushTag()
{
    Tag tag;
    tag.mName = mCurrentName.mName;
    tag.mPrefix = mCurrentName.mPrefix;
    tag.mLocalName = mCurrentName.mLocalName;
    tag.mNamespaceURI = mCurrentName.mNamespaceURI;
    tag.mPreviousMappingSize = mNamespaceMappings.size();
    mOpenTags.push_back(tag);
}
void XMLReader::PopTag()
{
    if (!mOpenTags.empty()) mOpenTags.pop_back();
}

// ---- XMLReader Name helpers ----
void XMLReader::Name::SetName(const char * name)
{
    if (name) {
        mName.clear();
        while (*name) mName += static_cast<UnicodeChar>(*name++);
    }
    DivideName();
}

void XMLReader::Name::SetName(const UnicodeChar * name)
{
    if (name) {
        mName.clear();
        while (*name) mName += *name++;
    }
    DivideName();
}

void XMLReader::Name::SetName(const UnicodeString & name)
{
    mName = name;
    DivideName();
}

void XMLReader::Name::DivideName()
{
    size_t colon = mName.find(':');
    if (colon != UnicodeString::npos) {
        mPrefix = mName.substr(0, colon);
        mLocalName = mName.substr(colon + 1);
    } else {
        mPrefix.clear();
        mLocalName = mName;
    }
}

void XMLReader::Name::Clear()
{
    mName.clear();
    mPrefix.clear();
    mLocalName.clear();
    mNamespaceURI.clear();
}

// ---- XMLReader::SkippingContent ----
XMLReader::SkippingContent::SkippingContent(XMLReader & reader)
    : mReader(reader), mWasSkippingContent(reader.mSkipContent)
{
    reader.mSkipContent = true;
}

XMLReader::SkippingContent::~SkippingContent()
{
    mReader.mSkipContent = mWasSkippingContent;
}

// ---- XMLReader static helpers ----
bool XMLReader::IsInRange(UnicodeChar c, const UniCharRange ranges[], size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (c >= ranges[i].mLowChar && c <= ranges[i].mHighChar) return true;
    }
    return false;
}

bool XMLReader::IsBaseChar(UnicodeChar c)
{
    return IsInRange(c, sBaseCharRanges, sizeof(sBaseCharRanges)/sizeof(sBaseCharRanges[0]));
}

bool XMLReader::IsIdeographic(UnicodeChar c)
{
    return (c >= 0x4E00 && c <= 0x9FA5) || c == 0x3007 || (c >= 0x3021 && c <= 0x3029);
}

bool XMLReader::IsLetter(UnicodeChar c) { return IsBaseChar(c) || IsIdeographic(c); }
bool XMLReader::IsDigit(UnicodeChar c)
{
    return IsInRange(c, sDigitRanges, sizeof(sDigitRanges)/sizeof(sDigitRanges[0]));
}
bool XMLReader::IsWhitespace(UnicodeChar c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
bool XMLReader::IsCombiningChar(UnicodeChar c)
{
    return IsInRange(c, sCombiningCharRanges, sizeof(sCombiningCharRanges)/sizeof(sCombiningCharRanges[0]));
}
bool XMLReader::IsNameChar(UnicodeChar c)
{
    return IsLetter(c) || IsDigit(c) || c == '.' || c == '-' || c == '_' || c == ':' || IsCombiningChar(c);
}
bool XMLReader::IsNameToken(const UnicodeString & token)
{
    if (token.empty()) return false;
    for (size_t i = 0; i < token.size(); ++i) {
        if (!IsNameChar(token[i])) return false;
    }
    return true;
}
bool XMLReader::IsName(const UnicodeString & name)
{
    if (name.empty()) return false;
    UnicodeChar c = name[0];
    if (!IsLetter(c) && c != '_' && c != ':') return false;
    for (size_t i = 1; i < name.size(); ++i) {
        if (!IsNameChar(name[i])) return false;
    }
    return true;
}

bool XMLReader::Equals(const UnicodeString & a, const char * b)
{
    if (!b) return a.empty();
    size_t i = 0;
    for (; i < a.size() && b[i]; ++i) {
        if (a[i] != static_cast<UnicodeChar>(b[i])) return false;
    }
    return i == a.size() && b[i] == '\0';
}

bool XMLReader::StartsWith(const UnicodeChar * haystack, const char * needle)
{
    if (!needle) return true;
    if (!haystack) return false;
    while (*needle) {
        if (*haystack != static_cast<UnicodeChar>(*needle)) return false;
        ++haystack;
        ++needle;
    }
    return true;
}

} // namespace LlamaXML


// ---- operator<< for OutputStream ----
LlamaXML::OutputStream & operator << (LlamaXML::OutputStream & stream, const char * s)
{
    if (s) {
        stream.WriteData(s, (LlamaXML::uint32_t)strlen(s));
    }
    return stream;
}


// ---- operator<< for XMLWriter ----
LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, const char * s)
{
    if (s) output.WriteString(s);
    return output;
}

LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, const std::string & s)
{
    output.WriteString(s.c_str());
    return output;
}

LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, const LlamaXML::UnicodeString & s)
{
    output.WriteString(s);
    return output;
}

LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, bool n)
{
    output.WriteString(n ? "true" : "false");
    return output;
}

LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, int n)
{
    char buf[32];
    sprintf_s(buf, "%d", n);
    output.WriteString(buf);
    return output;
}

LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, unsigned int n)
{
    char buf[32];
    sprintf_s(buf, "%u", n);
    output.WriteString(buf);
    return output;
}

LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, long n)
{
    char buf[32];
    sprintf_s(buf, "%ld", n);
    output.WriteString(buf);
    return output;
}

LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, unsigned long n)
{
    char buf[32];
    sprintf_s(buf, "%lu", n);
    output.WriteString(buf);
    return output;
}

LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, unsigned long long n)
{
    char buf[64];
    sprintf_s(buf, "%llu", n);
    output.WriteString(buf);
    return output;
}

LlamaXML::XMLWriter & operator << (LlamaXML::XMLWriter & output, double n)
{
    char buf[64];
    sprintf_s(buf, "%g", n);
    output.WriteString(buf);
    return output;
}
