/**
 * \file RestDB.hpp
 * \author Malte Langosz
 * \brief Fuctions to communicate with the database
 **/

#ifndef XROCK_GUI_MODEL_REST_DB_HPP
#define XROCK_GUI_MODEL_REST_DB_HPP

#include <configmaps/ConfigMap.hpp>
#include <xdbi/Client.hpp>
#include <xtypes_generator/XTypeRegistry.hpp>
#include <memory>
#include "DBInterface.hpp"


namespace xrock_gui_model {

  class RestDB : public DBInterface {

  public:
    RestDB();
    ~RestDB();

    virtual std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain);
    virtual std::vector<std::string> requestVersions(const std::string &domain, const std::string &model);
    virtual configmaps::ConfigMap requestModel(const std::string &domain,
                                               const std::string &model,
                                               const std::string &version,
                                               const bool limit = false);
    virtual bool storeModel(const configmaps::ConfigMap &map);

    virtual void set_dbAddress(const std::string &_dbAddress);
    virtual void set_dbUser(const std::string &_dbUser);
    virtual void set_dbPassword(const std::string &_dbPassword);
    virtual void set_dbGraph(const std::string &_dbGraph);
    //virtual void set_dbPath(const fs::path &_dbPath);


  private:
    std::unique_ptr<xdbi::Client> client;
    xtypes::XTypeRegistryPtr registry;
  };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_REST_DB_HPP
