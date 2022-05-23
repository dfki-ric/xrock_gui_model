/**
 * \file ModelWidget.hpp
 * \author Malte Langosz
 * \brief
 **/

#ifndef XROCK_GUI_MODEL_MODEL_WIDGET_HPP
#define XROCK_GUI_MODEL_MODEL_WIDGET_HPP

#include <mars/main_gui/BaseWidget.h>
#include <configmaps/ConfigMap.hpp>

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QLabel>

namespace bagel_gui {
  class BagelGui;
}

class ModelConfig {
  public:
    std::string name;
    std::string data;
};


namespace xrock_gui_model {
  class ModelLib;

  class ModelWidget : public mars::main_gui::BaseWidget {
    Q_OBJECT

  public:
    ModelWidget(mars::cfg_manager::CFGManagerInterface *cfg,
                bagel_gui::BagelGui *bagelGui, ModelLib *mainLib,
                QWidget *parent = 0);
    ~ModelWidget();
    void createMap(configmaps::ConfigMap *m);
    void loadModel(const std::string &file);
    void loadModel(configmaps::ConfigMap &m);
    void loadGraph(configmaps::ConfigMap &map);
    void saveGraph(configmaps::ConfigMap &output,
                   configmaps::ConfigMap &interfaceMap,
                   configmaps::ConfigMap &descriptionMap,
                   const std::string &saveDomain);
    void clear();
    void setModelInfo(configmaps::ConfigMap &model);
    void deinit();
    void setEdition(const std::string &domain);
    void editLocalMap();
    void editDescription();

  public slots:
    void checkMechanics(int v);
    void checkElectronics(int v);
    void checkSoftware(int v);
    void checkBehavior(int v);
    void checkComputation(int v);
    void layoutsClicked(const QModelIndex &index);
    void addRemoveLayout();
    void loadModel();
    void saveModel();
    void storeModel();
    void requestModel();
    void loadType(const std::string &domain, const std::string &name,
                  const std::string &version);
    void updateModelInfo();
    void getModelInfo(std::string *domain, std::string *name, std::string *verison);
    void openUrl(const QUrl &);
    void validateYamlSyntax();

  private:
    bagel_gui::BagelGui *bagelGui;
    ModelLib *mainLib;
    QListWidget *layouts;
    std::string currentLayout, modelPath, edition;
    std::map<std::string, QCheckBox*> checkMap;
    //QListWidget *nodeTypeView;
    //std::vector<std::string> nodeTypes;
    //std::string newNode;
    configmaps::ConfigMap localMap, layoutMap;
    QLineEdit *name, *type, *version, *domain, *layoutName, *maturity;
    QLineEdit *projectName, *designedBy;
    QTextEdit *includes, *data, *interfaces;
    QLabel *dataStatusLabel;
    configmaps::ConfigMap interfaceMap;
    bool ignoreUpdate;
    std::vector<std::string> xrockConfigFilter;
    void handleEditionLayout();
    void updateCurrentLayout();

    // this function is called from loadGraph()
    void loadNode(configmaps::ConfigMap &node, configmaps::ConfigMap &config);
    configmaps::ConfigMap getDefaultConfig(const std::string &domain, const std::string &name, const std::string &version);

  };

} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_MODEL_WIDGET_HPP
