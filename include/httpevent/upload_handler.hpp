#ifndef UPLOAD_HANDLER_HPP
#define UPLOAD_HANDLER_HPP

#include "libpoco.hpp"



namespace httpevent {

    class upload_handler : public Poco::Net::PartHandler {
    public:

        struct fileinfo {
            std::string name, filename, type, savepath, webpath, message;
            double size;
            bool ok;
        };
    public:
        upload_handler() = delete;

        upload_handler(
                const std::string& allowName
                , const std::string& allowType = Poco::Util::Application::instance().config().getString("http.uploadAllowType", "image/png|image/jpeg|image/gif|image/webp|application/zip")
                , const std::string& uploadDirectory = Poco::Util::Application::instance().config().getString("http.uploadDirectory", "/var/httpevent/www/upload")
                , double allowMaxSize = Poco::Util::Application::instance().config().getDouble("http.uploadMaxSize", 1048576))
        : data()
        , allowName(allowName)
        , allowType(allowType)
        , directory(uploadDirectory)
        , allowMaxSize(allowMaxSize) {

        }
        upload_handler(const upload_handler& orig) = delete;
        virtual ~upload_handler() = default;

        void handlePart(const Poco::Net::MessageHeader & header, std::istream & stream) {

            Poco::StringTokenizer nameST(this->allowName, "|", Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY)
                    , typeST(this->allowType, "|", Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);


            std::string type = header.get("Content-Type");
            std::string disposition = header.get("Content-Disposition");
            std::string v;
            Poco::Net::NameValueCollection nvc;
            Poco::Net::HTTPMessage::splitParameters(disposition, v, nvc);

            httpevent::upload_handler::fileinfo fileinfo;

            fileinfo.filename = Poco::Net::HTTPCookie::escape(nvc.get("filename"));
            fileinfo.name = Poco::Net::HTTPCookie::escape(nvc.get("name"));
            fileinfo.type = type;
            fileinfo.ok = false;
            fileinfo.message = "Not allow upload.";
            fileinfo.savepath = "";
            fileinfo.webpath = "";
            fileinfo.size = 0;

            if (typeST.has(fileinfo.type) && nameST.has(fileinfo.name)) {

                Poco::LocalDateTime now;
                fileinfo.savepath = Poco::format("%[0]s/%[1]s"
                        , this->directory
                        , Poco::DateTimeFormatter::format(now, "%Y/%m/%d/%H"));
                Poco::File uploaddir(fileinfo.savepath);
                if (!uploaddir.exists()) {
                    uploaddir.createDirectories();
                }

                Poco::Path path(fileinfo.filename);
                Poco::MD5Engine md5;
                md5.update(Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::HTTP_FORMAT) + fileinfo.filename);
                fileinfo.savepath += Poco::format("/%[0]s.%[1]s", Poco::MD5Engine::digestToHex(md5.digest()), path.getExtension());


                Poco::FileOutputStream fileoutstream(fileinfo.savepath);


                bool allowSave = true;
                int ch = stream.get();
                double size(0);

                while (allowSave && ch >= 0) {
                    if (size <= this->allowMaxSize) {
                        fileoutstream.put((char) ch);
                        ch = stream.get();
                        size += 1;

                    } else {
                        allowSave = false;
                        fileinfo.message += " The file size is too big.";
                    }
                }

                if (allowSave) {
                    fileinfo.webpath = Poco::replace(fileinfo.savepath, this->directory, std::string());
                    fileinfo.size = size;
                    fileinfo.ok = true;
                    fileinfo.message = "Allow upload.";
                } else {
                    Poco::File tempFile(fileinfo.savepath);
                    tempFile.remove();
                }
            }

            this->data.push_back(fileinfo);

        }

        const std::vector<fileinfo>& get_data() {
            return this->data;
        }

    private:
        std::vector<fileinfo> data;
        std::string allowName, allowType, directory;
        double allowMaxSize;
    };

}

#endif /* UPLOAD_HANDLER_HPP */

