/**
 * \file DBInterface.hpp
 * \author Malte Langosz
 * \brief Fuctions to communicate with databases
 **/

#ifndef XROCK_GUI_MODEL_XROCK_DB_INTERFACE_HPP
#define XROCK_GUI_MODEL_XROCK_DB_INTERFACE_HPP

#include <configmaps/ConfigMap.hpp>


namespace xrock_gui_model {

  class DBInterface {

  public:
    DBInterface() {}
    ~DBInterface() {}

    virtual std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain) = 0;
    virtual  std::vector<std::string> requestVersions(const std::string &domain, const std::string &model) = 0;
    virtual  configmaps::ConfigMap requestModel(const std::string &domain,
                                                const std::string &model,
                                                const std::string &version,
                                                const bool limit = false) = 0;
    virtual bool storeModel(const configmaps::ConfigMap &map) = 0;

    virtual void set_dbAddress(const std::string &_dbAddress) = 0;

  };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_XROCK_DB_INTERFACE_HPP
