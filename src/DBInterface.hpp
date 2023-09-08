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
        DBInterface() = default;
        virtual ~DBInterface() = default;

        /**
         * @brief Requests a list of models and their associated domain names.
         *
         * This function retrieves a list of models and their corresponding domain names.
         *
         * @param domain The domain for which to retrieve the list of models.
         * @return A vector of pairs where each pair contains the model name and its associated domain name.
         */
        virtual std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain) = 0;

        /**
         * @brief Requests a list of versions for a specific model within a domain.
         *
         * This function retrieves a list of available versions for a given model within a specific domain.
         *
         * @param domain The domain where the model is located.
         * @param model The name of the model for which to retrieve the versions.
         * @return A vector of strings containing the available versions for the specified model.
         */
        virtual std::vector<std::string> requestVersions(const std::string &domain, const std::string &model) = 0;

        /**
         * @brief Requests a specific model with the specified version from the given domain.
         *
         * This function retrieves a model with a specific version from the specified domain.
         *
         * @param domain The domain where the model is located.
         * @param model The name of the model to retrieve.
         * @param version The version of the model to retrieve.
         * @param limit If true, limits the model retrieval based on a certain condition.
         * @return A ConfigMap object containing the requested model.
         */
        virtual configmaps::ConfigMap requestModel(const std::string &domain,
                                                   const std::string &model,
                                                   const std::string &version,
                                                   const bool limit = false) = 0;
        /**
        * @brief Stores a model in the database.
        *
        * This function stores model in the database .
        *
        * @param map The ConfigMap containing the model to be stored.
        * @return True if the model is successfully stored; otherwise, false.
        */
        virtual bool storeModel(const configmaps::ConfigMap &map) = 0;

        /**
         * @brief Removes a model from the database using its URI.
         *
         * This function removes a model from the database based on its URI.
         *
         * @param uri The URI of the model to be removed.
         * @return True if the model is successfully removed; otherwise, false.
         */
        virtual bool removeModel(const std::string &uri) = 0;

        /**
         * @brief Sets the database working graph.
         *
         * This function sets the working graph of the database used.
         *
         * @param _dbGraph The working graph to be set.
         */
        virtual void setDbGraph(const std::string &_dbGraph){};

        /**
         * @brief Sets the database address.
         *
         * This function sets the path of the database used.
         *
         * @param _dbAddress The database address to be set.
         */
        virtual void setDbAddress(const std::string &_dbAddress){};

        /**
         * @brief Sets the database path.
         *
         * This function sets the path of the database used.
         *
         * @param _dbPath The filesystem path of the database to be set.
         */
        virtual void setDbPath(const fs::path&_dbPath){};

        /**
         * @brief Returns true if the backend is connected (ready to use)
         *     
         */
        virtual bool isConnected() { return false; };

        /**
         * @brief Retrieves a map of the default properties of a component model.
         */
        virtual configmaps::ConfigMap getPropertiesOfComponentModel() = 0;
        /**
         * @brief Retrieves a map of the default domains of a component model.
         */
        virtual std::vector<std::string> getDomains() = 0;
        /**
         * @brief Retrieves a basic json of an empty component model.
         */
        virtual configmaps::ConfigMap getEmptyComponentModel() = 0;
        
        /**
        * @brief Builds a module and saves it to database for a given model URI.
        *
        * This function builds a module for a given model URI and saves it to database.
        *
        * @param uri The URI of the model to build module for and store in the database.
        * @param moduleName The name of the module to be built.
        * @param selectedImplementationsUris A map of selected implementations uris for unresolved abstracts within the model.
        * @return True if the module is successfully built and saved to database; otherwise, false.
        */
        virtual bool buildModule(const std::string &uri, const std::string &moduleName, const std::map <std::string,std::string>& selected_implementations ) { return false; };
        
        /**
         * @brief Retrieves a map of unresolved abstracts for a given URI.
         *
         * This function retrieves a map of unresolved abstracts associated with the given URI.
         *
         * @param uri The URI for which to retrieve the unresolved abstracts.
         * @return A map where each key represents an unresolved abstract uri,name and version and the value is a list of implementations uri,name and version.
         */
        virtual configmaps::ConfigMap getUnresolvedAbstracts(const std::string& uri) { return {}; };
    };
} // end of namespace xrock_gui_model

