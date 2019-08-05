/**
 * \file FileDB.hpp
 * \author Malte Langosz
 * \brief Fuctions to communicate with the database
 **/

#ifndef XROCK_GUI_MODEL_XROCK_DB_HPP
#define XROCK_GUI_MODEL_XROCK_DB_HPP

#include <configmaps/ConfigMap.hpp>


namespace xrock_gui_model {

  class XRockDB {

  public:
    XRockDB();
    ~XRockDB();

    static std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain);
    static std::vector<std::string> requestVersions(const std::string &domain, const std::string &model);
    static configmaps::ConfigMap requestModel(const std::string &domain,
                                              const std::string &model,
                                              const std::string &version,
                                              const bool limit = false);
    static bool storeModel(const configmaps::ConfigMap &map);

    static void set_dbAddress(const std::string &_dbAddress);

  private:
    static std::string dbAddress;


  };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_XROCK_DB_HPP
