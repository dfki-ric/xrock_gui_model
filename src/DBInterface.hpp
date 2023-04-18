/**
 * \file DBInterface.hpp
 * \author Malte Langosz
 * \brief Fuctions to communicate with databases
 **/

#pragma once
#include <configmaps/ConfigMap.hpp>
#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <boost/filesystem.hpp>
    namespace fs = boost::filesystem;
#endif
namespace xrock_gui_model
{

    class DBInterface
    {

    public:
        DBInterface() {}
        virtual ~DBInterface() {}

        virtual std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain) = 0;
        virtual std::vector<std::string> requestVersions(const std::string &domain, const std::string &model) = 0;
        virtual configmaps::ConfigMap requestModel(const std::string &domain,
                                                   const std::string &model,
                                                   const std::string &version,
                                                   const bool limit = false) = 0;
        virtual bool storeModel(const configmaps::ConfigMap &map) = 0;
        virtual bool removeModel(const std::string &uri) = 0;
        virtual void setDbGraph(const std::string &_dbGraph){};
        virtual void setDbAddress(const std::string &_dbAddress){};
        virtual void setDbPath(const fs::path&_dbPath){};
        virtual bool isConnected() { return false; };

        virtual configmaps::ConfigMap getPropertiesOfComponentModel() = 0;
        virtual std::vector<std::string> getDomains() = 0;
        virtual configmaps::ConfigMap getEmptyComponentModel() = 0;
    };
} // end of namespace xrock_gui_model

