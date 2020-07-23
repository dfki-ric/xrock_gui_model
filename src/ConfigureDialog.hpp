/**
 * \file ConfigureDialog.hpp
 * \author Malte Langosz
 * \brief Displays known configures of a component and allows to select one
 **/

#ifndef XROCK_GUI_MODEL_CONFIGURE_DIALOG_HPP
#define XROCK_GUI_MODEL_CONFIGURE_DIALOG_HPP

#include <configmaps/ConfigMap.hpp>

#include <QDialog>
#include <QTextEdit>
#include <QLabel>

namespace mars {
  namespace config_map_gui {
    class DataWidget;
  }
}

namespace xrock_gui_model {

  class ConfigureDialog : public QDialog {
    Q_OBJECT

  public:
    ConfigureDialog(configmaps::ConfigMap *configuration,
                    configmaps::ConfigMap &env,
                    const std::string &type, bool onlyMap=false,
                    bool noTreeEdit=false, configmaps::ConfigMap *dropdown = NULL,
                    std::string fileName = "");
    ConfigureDialog(std::string *text);
    ~ConfigureDialog();

  protected slots:
    void textChanged();
    void timerEvent(QTimerEvent *event);

  private:
    mars::config_map_gui::DataWidget *dw;
    QTextEdit *configFile, *configMapEdit;
    configmaps::ConfigMap *configuration;
    std::string configFileName, configFileContent, *text;
    QLabel *statusLabel;
    long ticks, lastTextChanged;
    bool textOnly;
  };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_CONFIGURE_DIALOG_HPP
