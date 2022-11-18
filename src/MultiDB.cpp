#include "MultiDB.hpp"
#include <mars/utils/misc.h>
#include <xtypes/ComponentModel.hpp>

#include <iostream>
#include <iomanip>
#include <ctime>

using namespace configmaps;
using namespace xtypes;

namespace xrock_gui_model
{

    MultiDB::MultiDB(const nl::json &config) : registry(new xtypes::XTypeRegistry())
    {
        registry->register_class<ComponentModel>();
        multidb = std::make_unique<xdbi::MultiDbClient>(registry, config);
    }

    MultiDB::~MultiDB()
    {
    }

    std::vector<std::pair<std::string, std::string>> MultiDB::requestModelListByDomain(const std::string &domain)
    {
        nl::json props;
        props["domain"] = mars::utils::toupper(domain);
        const std::vector<XTypePtr> models = multidb->find("ComponentModel", props, 0);
        std::vector<std::pair<std::string, std::string>> out;
        out.reserve(models.size());
        std::transform(models.begin(), models.end(), std::back_inserter(out), [&](auto &model)
                       {if (!model->has_property("version"))throw std::runtime_error("Model of ComponentModel type has no 'version' property!"); 
                       return std::make_pair(model->get_property("name"), model->get_property("type")); });
        return out;
    }

    std::vector<std::string> MultiDB::requestVersions(const std::string &domain, const std::string &model)
    {
        // ToDo as soon as modelDeepness is integrated, we can simplify this
        // request["modelDeepness"] = "versionOnly";
        nl::json props;
        props["name"] = model;
        props["domain"] = mars::utils::toupper(domain);
        const std::vector<XTypePtr> models = multidb->find("ComponentModel", props);
        std::vector<std::string> out;
        out.reserve(models.size());
        std::transform(models.begin(), models.end(), std::back_inserter(out), [&](auto &model)
                       {if (!model->has_property("version"))throw std::runtime_error("Model of ComponentModel type has no 'version' property!");
                       return model->get_property("version"); });
        return out;
    }

    ConfigMap MultiDB::requestModel(const std::string &domain,
                                    const std::string &model,
                                    const std::string &version,
                                    const bool limit)
    {
        nl::json props;
        props["domain"] = mars::utils::toupper(domain);
        props["name"] = model;
        if (version.size() > 0)
        {
            props["version"] = version;
        }

        std::vector<XTypePtr> xtypes = multidb->find("ComponentModel", props, limit ? 3 : -1);
        if (xtypes.size() == 0)
        {
            std::cerr << "ComponentModel with props: " << props << " not loaded" << std::endl;
            abort();
        }
        return ConfigMap::fromJsonString(std::static_pointer_cast<ComponentModel>(xtypes[0])->export_to_basic_model());
    }

    bool MultiDB::storeModel(const ConfigMap &map)
    {
        ConfigMap model = ConfigMap(map);
        // here we assume that the maps fits to the json representation required by the db
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
        auto date = oss.str();

        for (unsigned int i = 0; i < model["versions"].size(); ++i)
        {
            model["versions"][(int)i]["date"] = date;
        }


        std::string serialized_model = model.toJsonString();

        auto load_missing_models = [&](const nl::json &u) -> ComponentModelPtr
        {
            ComponentModel dummy;
            dummy.set_properties(u);
            const std::string &uri(dummy.uri());
            std::vector<XTypePtr> full_models = multidb->find("ComponentModel", nl::json{{"uri", uri}}); //, limit_recursion=True, recursion_depth=3)
            assert(full_models.size() == 1);
            return std::static_pointer_cast<ComponentModel>(full_models[0]);
        };

        // # import from JSON and export to DB
        std::vector<ComponentModelPtr> models = ComponentModel::import_from_basic_model(serialized_model, load_missing_models);
        std::vector<XTypePtr> db_models{};
        std::transform(models.begin(), models.end(), std::back_inserter(db_models), [&](auto &model)
                       {return std::static_pointer_cast<XType>(model); });

        try
        {
            multidb->update(db_models);
        }
        catch (...)
        {
            std::cerr << "ERROR: Problem with database communication" << std::endl;
            return false;
        }
        return true;
    }

    void MultiDB::set_dbGraph(const std::string &_dbGraph)
    {
        multidb->setWorkingGraph(_dbGraph);
    }


} // end of namespace xrock_gui_model
