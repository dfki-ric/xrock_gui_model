#pragma once
#include "../ConfigureDialogLoader.hpp"

#include "../XRockGUI.hpp"
#include <configmaps/ConfigMap.hpp>

#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <bagel_gui/BagelGui.hpp>
namespace mars
{
    namespace config_map_gui
    {
        class DataWidget;
    }
}
namespace bagel_gui
{
    class BagelModel;
}

namespace xrock_gui_model
{
    
    class XRockGUI;

    class ROCKTASKConfig : public QDialog
    {
        Q_OBJECT

    public:
        ROCKTASKConfig(XRockGUI *xrockGui, configmaps::ConfigMap *configuration,
                       configmaps::ConfigMap &env,
                       configmaps::ConfigMap &globalConfig,
                       const std::string &type, bool onlyMap = false,
                       bool noTreeEdit = false, configmaps::ConfigMap *dropdown = NULL,
                       std::string fileName = "");
        explicit ROCKTASKConfig(std::string *text);
        ~ROCKTASKConfig();

        void updateDataWidget(configmaps::ConfigMap &newConfig);
        void initEditablePattern(configmaps::ConfigMap &newConfig);
        void initCheckablePattern(configmaps::ConfigMap &newConfig);
        void initDropDownPattern(configmaps::ConfigMap &newConfig);
        void postUpdateCheckablePattern();
        configmaps::ConfigAtom propertyToConfigAtom(configmaps::ConfigItem &property);

    protected slots:
        void textChanged();
        void mapChanged();
        void timerEvent(QTimerEvent *event);

    private:
        configmaps::ConfigMap nodeMap;
        mars::config_map_gui::DataWidget *dw;
        XRockGUI *xrockGui;
        QTextEdit *configFile, *configMapEdit;
        configmaps::ConfigMap *configuration, dwConfig, globalConfig;
        std::string configFileName, configFileContent, *text, contextNodeName;
        std::vector<std::string> dropDownPattern;
        std::vector<std::string> checkablePattern;
        std::vector<std::vector<std::string>> dropDownValues;
        QLabel *statusLabel;
        long ticks, lastTextChanged;
        bool textOnly;
    };

    class ROCKTASKConfigLoader : public ConfigureDialogLoader
    {
    public:
        ROCKTASKConfigLoader(XRockGUI *xrockGui) : xrockGui(xrockGui) {}
        virtual ~ROCKTASKConfigLoader() override {}
        virtual QDialog* createDialog(configmaps::ConfigMap *configuration,
                                      configmaps::ConfigMap &env,
                                      configmaps::ConfigMap &globalConfig) override;

    private:
        XRockGUI *xrockGui;
    };

} // end of namespace xrock_gui_model

