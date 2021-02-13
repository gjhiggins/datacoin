// detector.h
//
// MIT License
//
// Copyright (c) 2017 Heikki Hellgren <heiccih@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef FILE_MIME_DETECTOR_H_
#define FILE_MIME_DETECTOR_H_

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>

class Detector {
public:
    /// Detect mimetype based on filename
    ///
    /// \param[in] file_name std::string File name to detect
    ///
    /// \return std::string
    std::string detect(const std::string& file_name) {
        std::string extension = get_file_extension(file_name);
        if (!extension.empty()) {
            std::string mime = detect_from_extension(extension);
            if (!mime.empty()) {
                return mime;
            }
        }

        std::fstream file(file_name, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file || !file.is_open()) {
            return detect(file);
        }

        return "application/octet-stream";
    }

    /// Detect mimetype based on file stream
    ///
    /// \param[in] file std::fstream file File to detect
    ///
    /// \return std::string
    std::string detect(std::fstream& file) {
        std::string hex = get_magic_hex(file);
        // std::cout << "Magic hex: " << hex << std::endl;
        std::string mime = detect_from_hex(hex);
        if (!mime.empty()) {
            return mime;
        }

        return "application/octet-stream";
    }

    /// Detect mimetype based on raw data
    ///
    /// \param[in] std::vector<unsigned char> Data to detect
    ///
    /// \return std::string
    std::string detect(const std::vector<unsigned char>& data) {
        std::string hex = get_magic_hex(data);
        // std::cout << "Magic hex: " << hex << std::endl;
        std::string mime = detect_from_hex(hex);
        if (!mime.empty()) {
            return mime;
        }

        return "application/octet-stream";
    }

private:
    /// Return file extension from file name
    ///
    /// \param[in] file_name std::string File name to detect extension from
    ///
    /// \return std::string
    std::string get_file_extension(const std::string& file_name) {
        auto idx = file_name.rfind('.');
        if (idx != std::string::npos) {
            return file_name.substr(idx + 1);
        }
        return "";
    }

    /// Get magic hex number from given file
    ///
    /// \param[in] file std::fstream File to read magic number from
    ///
    /// \return std::string
    std::string get_magic_hex(std::fstream& file) {
        std::size_t max_length = m_hex_types.front().first.length();
        file.seekg(0, std::ios::end);
        std::size_t file_size = file.tellg();
        if (file_size < max_length) {
            max_length = file_size;
        }

        file.seekg(0, std::ios::beg);
        char* x = new char[max_length];
        file.read(x, max_length);
        std::stringstream ss;
        for (int i = 0; i < (int)max_length; i++) {
            ss << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(x[i] & 0xff);
        }
        return ss.str();
    }

    /// Get magic hex number from given data
    ///
    /// \param[in] std::vector<unsigned char>& data Data to read magic number from
    ///
    /// \return std::string
    std::string get_magic_hex(const std::vector<unsigned char>& data) {
        std::size_t max_length = m_hex_types.front().first.length();
        if (data.size() < max_length) {
            max_length = data.size();
        }
        // char* x = new char[max_length];
        // file.read(x, max_length);
        std::stringstream ss;
        for (int i = 0; i < (int)max_length; i++) {
            ss << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(data[i] & 0xff);
        }
        return ss.str();
    }

    /// Detect mimetype from given extension
    ///
    /// \param[in] extension std::string File extension
    ///
    /// \return std::string
    std::string detect_from_extension(const std::string& extension) {
        auto it = std::find_if(
            m_extension_types.begin(), m_extension_types.end(),
            [&extension](const std::pair<std::string, std::string>& elem) { return elem.first == extension; });
        if (it == m_extension_types.end()) {
            return "";
        }
        return it->second;
    }

    /// Detect mimetype from given magic hex number
    ///
    /// \param[in] hex std::string Magic number for file
    ///
    /// \return std::string
    std::string detect_from_hex(const std::string& hex) {
        for (auto regex_str : m_hex_types) {
            std::regex m(regex_str.first + ".*");
            if (std::regex_match(hex, m)) {
                return regex_str.second;
            }
        }

        return "";
    }

    /// Length comparison for hex types
    template <typename T1, typename T2> struct length_compare {
        typedef std::pair<T1, T2> type;
        bool operator()(type const& a, type const& b) const { return a.first.length() > b.first.length(); }
    };

    /// Extension type -> mime type mapping
    std::vector<std::pair<std::string, std::string>> m_extension_types = {
        {"evy", "application/envoy"},
        {"fif", "application/fractals"},
        {"spl", "application/futuresplash"},
        {"hta", "application/hta"},
        {"acx", "application/internet-property-stream"},
        {"hqx", "application/mac-binhex40"},
        {"doc", "application/msword"},
        {"dot", "application/msword"},
        {"*", "application/octet-stream"},
        {"bin", "application/octet-stream"},
        {"class", "application/octet-stream"},
        {"dms", "application/octet-stream"},
        {"exe", "application/octet-stream"},
        {"lha", "application/octet-stream"},
        {"lzh", "application/octet-stream"},
        {"oda", "application/oda"},
        {"axs", "application/olescript"},
        {"pdf", "application/pdf"},
        {"prf", "application/pics-rules"},
        {"p10", "application/pkcs10"},
        {"crl", "application/pkix-crl"},
        {"ai", "application/postscript"},
        {"eps", "application/postscript"},
        {"ps", "application/postscript"},
        {"rtf", "application/rtf"},
        {"setpay", "application/set-payment-initiation"},
        {"setreg", "application/set-registration-initiation"},
        {"xla", "application/vnd.ms-excel"},
        {"xlc", "application/vnd.ms-excel"},
        {"xlm", "application/vnd.ms-excel"},
        {"xls", "application/vnd.ms-excel"},
        {"xlt", "application/vnd.ms-excel"},
        {"xlw", "application/vnd.ms-excel"},
        {"msg", "application/vnd.ms-outlook"},
        {"sst", "application/vnd.ms-pkicertstore"},
        {"cat", "application/vnd.ms-pkiseccat"},
        {"stl", "application/vnd.ms-pkistl"},
        {"pot", "application/vnd.ms-powerpoint"},
        {"pps", "application/vnd.ms-powerpoint"},
        {"ppt", "application/vnd.ms-powerpoint"},
        {"mpp", "application/vnd.ms-project"},
        {"wcm", "application/vnd.ms-works"},
        {"wdb", "application/vnd.ms-works"},
        {"wks", "application/vnd.ms-works"},
        {"wps", "application/vnd.ms-works"},
        {"hlp", "application/winhlp"},
        {"bcpio", "application/x-bcpio"},
        {"cdf", "application/x-cdf"},
        {"z", "application/x-compress"},
        {"tgz", "application/x-compressed"},
        {"cpio", "application/x-cpio"},
        {"csh", "application/x-csh"},
        {"dcr", "application/x-director"},
        {"dir", "application/x-director"},
        {"dxr", "application/x-director"},
        {"dvi", "application/x-dvi"},
        {"gtar", "application/x-gtar"},
        {"gz", "application/x-gzip"},
        {"hdf", "application/x-hdf"},
        {"ins", "application/x-internet-signup"},
        {"isp", "application/x-internet-signup"},
        {"iii", "application/x-iphone"},
        {"js", "application/x-javascript"},
        {"latex", "application/x-latex"},
        {"mdb", "application/x-msaccess"},
        {"crd", "application/x-mscardfile"},
        {"clp", "application/x-msclip"},
        {"dll", "application/x-msdownload"},
        {"m13", "application/x-msmediaview"},
        {"m14", "application/x-msmediaview"},
        {"mvb", "application/x-msmediaview"},
        {"wmf", "application/x-msmetafile"},
        {"mny", "application/x-msmoney"},
        {"pub", "application/x-mspublisher"},
        {"scd", "application/x-msschedule"},
        {"trm", "application/x-msterminal"},
        {"wri", "application/x-mswrite"},
        {"cdf", "application/x-netcdf"},
        {"nc", "application/x-netcdf"},
        {"pma", "application/x-perfmon"},
        {"pmc", "application/x-perfmon"},
        {"pml", "application/x-perfmon"},
        {"pmr", "application/x-perfmon"},
        {"pmw", "application/x-perfmon"},
        {"p12", "application/x-pkcs12"},
        {"pfx", "application/x-pkcs12"},
        {"p7b", "application/x-pkcs7-certificates"},
        {"spc", "application/x-pkcs7-certificates"},
        {"p7r", "application/x-pkcs7-certreqresp"},
        {"p7c", "application/x-pkcs7-mime"},
        {"p7m", "application/x-pkcs7-mime"},
        {"p7s", "application/x-pkcs7-signature"},
        {"sh", "application/x-sh"},
        {"shar", "application/x-shar"},
        {"swf", "application/x-shockwave-flash"},
        {"sit", "application/x-stuffit"},
        {"sv4cpio", "application/x-sv4cpio"},
        {"sv4crc", "application/x-sv4crc"},
        {"tar", "application/x-tar"},
        {"tcl", "application/x-tcl"},
        {"tex", "application/x-tex"},
        {"texi", "application/x-texinfo"},
        {"texinfo", "application/x-texinfo"},
        {"roff", "application/x-troff"},
        {"t", "application/x-troff"},
        {"tr", "application/x-troff"},
        {"man", "application/x-troff-man"},
        {"me", "application/x-troff-me"},
        {"ms", "application/x-troff-ms"},
        {"ustar", "application/x-ustar"},
        {"src", "application/x-wais-source"},
        {"cer", "application/x-x509-ca-cert"},
        {"crt", "application/x-x509-ca-cert"},
        {"der", "application/x-x509-ca-cert"},
        {"pko", "application/ynd.ms-pkipko"},
        {"zip", "application/zip"},
        {"au", "audio/basic"},
        {"snd", "audio/basic"},
        {"mid", "audio/mid"},
        {"rmi", "audio/mid"},
        {"mp3", "audio/mpeg"},
        {"aif", "audio/x-aiff"},
        {"aifc", "audio/x-aiff"},
        {"aiff", "audio/x-aiff"},
        {"m3u", "audio/x-mpegurl"},
        {"ra", "audio/x-pn-realaudio"},
        {"ram", "audio/x-pn-realaudio"},
        {"wav", "audio/x-wav"},
        {"bmp", "image/bmp"},
        {"cod", "image/cis-cod"},
        {"gif", "image/gif"},
        {"ief", "image/ief"},
        {"jpe", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"jpg", "image/jpeg"},
        {"jfif", "image/pipeg"},
        {"svg", "image/svg+xml"},
        {"tif", "image/tiff"},
        {"tiff", "image/tiff"},
        {"ras", "image/x-cmu-raster"},
        {"cmx", "image/x-cmx"},
        {"ico", "image/x-icon"},
        {"pnm", "image/x-portable-anymap"},
        {"pbm", "image/x-portable-bitmap"},
        {"pgm", "image/x-portable-graymap"},
        {"ppm", "image/x-portable-pixmap"},
        {"rgb", "image/x-rgb"},
        {"xbm", "image/x-xbitmap"},
        {"xpm", "image/x-xpixmap"},
        {"xwd", "image/x-xwindowdump"},
        {"mht", "message/rfc822"},
        {"mhtml", "message/rfc822"},
        {"nws", "message/rfc822"},
        {"css", "text/css"},
        {"323", "text/h323"},
        {"htm", "text/html"},
        {"html", "text/html"},
        {"stm", "text/html"},
        {"uls", "text/iuls"},
        {"bas", "text/plain"},
        {"cxx", "text/plain"},
        {"cpp", "text/plain"},
        {"c", "text/plain"},
        {"h", "text/plain"},
        {"hpp", "text/plain"},
        {"txt", "text/plain"},
        {"rtx", "text/richtext"},
        {"sct", "text/scriptlet"},
        {"tsv", "text/tab-separated-values"},
        {"htt", "text/webviewhtml"},
        {"htc", "text/x-component"},
        {"etx", "text/x-setext"},
        {"vcf", "text/x-vcard"},
        {"mp2", "video/mpeg"},
        {"mpa", "video/mpeg"},
        {"mpe", "video/mpeg"},
        {"mpeg", "video/mpeg"},
        {"mpg", "video/mpeg"},
        {"mpv2", "video/mpeg"},
        {"mp4", "video/mp4"},
        {"mov", "video/quicktime"},
        {"qt", "video/quicktime"},
        {"lsf", "video/x-la-asf"},
        {"lsx", "video/x-la-asf"},
        {"asf", "video/x-ms-asf"},
        {"asr", "video/x-ms-asf"},
        {"asx", "video/x-ms-asf"},
        {"avi", "video/x-msvideo"},
        {"movie", "video/x-sgi-movie"},
        {"flr", "x-world/x-vrml"},
        {"vrml", "x-world/x-vrml"},
        {"wrl", "x-world/x-vrml"},
        {"wrz", "x-world/x-vrml"},
        {"xaf", "x-world/x-vrml"},
        {"xof", "x-world/x-vrml"},
    };

    /// HEX magic number -> mime type mapping
    std::vector<std::pair<std::string, std::string>> m_hex_types = {
        {"667479703367", "video/3gpp"},
        {"464F524D(.*){8}41494646", "audio/x-aiff"},
        {"3026B2758E66CF11A6D900AA0062CE6C", "audio/x-ms-wma"},
        {"52494646(.*){8}41564920", "video/x-msvideo"},
        {"424D", "image/bmp"},
        {"425A68", "application/x-bzip2"},
        {"FD377A585A000004E6D6B446", "application/x-lzh"},
        {"377ABCAF271C", "application/x-7z-compressed"},
        {"1F8B", "application/gzip"},
        {"526172211A070100", "application/x-rar-compressed"},
        {"526172211A0700", "application/x-rar-compressed"},
        {"504B0304", "application/zip"},
        {"504B0506", "application/zip"},
        {"504B0708", "application/zip"},
        {"4D534346", "application/vnd.ms-cab-compressed"},
        {"CAFEBABE", "application/java-vm"},
        {"213C617263683E", "application/x-debian-package"},
        {"3082", "application/x-x509-ca-cert"},
        {"41542654464F524D(.*){8}444A56", "image/vnd.djvu"},
        {"7801730D626260", "application/x-apple-diskimage"},
        {"4D5A", "application/x-msdownload"},
        {"474946383761", "image/gif"},
        {"474946383961", "image/gif"},
        {"00000100", "image/x-icon"},
        {"FFD8FFDB", "image/jpg"},
        {"FFD8FFE0(.*){4}4A4649460001", "image/jpeg"},
        {"FFD8FFE1(.*){4}457869660000", "image/jpeg"},
        {"4D546864", "audio/midi"},
        {"1A45DFA3", "video/webm"},
        {"FFFB", "audio/mpeg"},
        {"494433", "audio/mpeg"},
        {"000001BA", "video/mpeg"},
        {"47", "video/mpeg"},
        {"000001B3", "video/mpeg"},
        {"4F676753", "application/ogg"},
        {"25504446", "application/pdf"},
        {"89504E470D0A1A0A", "image/png"},
        {"38425053", "image/vnd.adobe.photoshop"},
        {"7B5C72746631", "application/rtf"},
        {"435753", "application/x-shockwave-flash"},
        {"465753", "application/x-shockwave-flash"},
        {"7573746172003030", "application/x-tar"},
        {"7573746172202000", "application/x-tar"},
        {"49492A00", "image/tiff"},
        {"4D4D002A", "image/tiff"},
        {"52494646(.*){8}57415645", "audio/x-wav"},
        {"52494646(.*){8}57454250", "image/webp"},
        {"774F4646", "font/woff"},
        {"774f4632", "font/woff2"},
        {"3c3f786d6c20", "application/xml"},
        {"3C21444F4354595045206874", "text/html"},
        {"4070726566697820666F6166", "application/x-turtle"},
        {"3C3F786D6C2076657273696F", "application/rdf+xml"}
    };
};

#endif // FILE_MIME_DETECTOR_H_
