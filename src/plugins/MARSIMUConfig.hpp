/**
 * \file MARSIMUConfig.hpp
 * \author Malte Langosz
 * \brief MARS IMU Task configuration dialog
 **/

#pragma once
#include "../ConfigureDialogLoader.hpp"

#include <configmaps/ConfigMap.hpp>

#include <QDialog>
#include <QTextEdit>
#include <QLabel>

namespace mars
{
    namespace config_map_gui
    {
        class DataWidget;
    }
}

namespace xrock_gui_model
{

    class MARSIMUConfig : public QDialog
    {
        Q_OBJECT

    public:
        MARSIMUConfig(configmaps::ConfigMap *configuration,
                      configmaps::ConfigMap &env,
                      configmaps::ConfigMap &globalConfig,
                      const std::string &type, bool onlyMap = false,
                      bool noTreeEdit = false, configmaps::ConfigMap *dropdown = NULL,
                      std::string fileName = "");
        explicit MARSIMUConfig(std::string *text);
        ~MARSIMUConfig();

    protected slots:
        void textChanged();
        void mapChanged();
        void timerEvent(QTimerEvent *event);

    private:
        mars::config_map_gui::DataWidget *dw;
        QTextEdit *configFile, *configMapEdit;
        configmaps::ConfigMap *configuration, dwConfig;
        std::string configFileName, configFileContent, *text;
        QLabel *statusLabel;
        long ticks, lastTextChanged;
        bool textOnly;
        std::vector<std::vector<std::string>> values;
    };

    class MARSIMUConfigLoader : public ConfigureDialogLoader
    {
    public:
        MARSIMUConfigLoader(){}
        virtual ~MARSIMUConfigLoader() override {}
        virtual QDialog* createDialog(configmaps::ConfigMap *configuration,
                                      configmaps::ConfigMap &env,
                                      configmaps::ConfigMap &globalConfig) override;
    };

} // end of namespace xrock_gui_model

