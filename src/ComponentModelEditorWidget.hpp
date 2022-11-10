/**
 * \file ComponentModelEditorWidget.hpp
 * \author Malte Langosz
 * \brief
 **/

#ifndef XROCK_GUI_MODEL_MODEL_WIDGET_HPP
#define XROCK_GUI_MODEL_MODEL_WIDGET_HPP

#include <mars/main_gui/BaseWidget.h>
#include <configmaps/ConfigMap.hpp>
#include "ComponentModelInterface.hpp"

#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QLabel>

namespace bagel_gui
{
    class BagelGui;
}

class ModelConfig
{
public:
    std::string name;
    std::string data;
};

namespace xrock_gui_model
{
    class XRockGUI;

    class  ComponentModelEditorWidget : public mars::main_gui::BaseWidget
    {
        Q_OBJECT

    public:
        ComponentModelEditorWidget(mars::cfg_manager::CFGManagerInterface *cfg,
                    bagel_gui::BagelGui *bagelGui, XRockGUI *xrockGui,
                    QWidget *parent = 0);
        ~ComponentModelEditorWidget();
        void clear();
        void deinit();
        void setEdition(const std::string &domain);

    public slots:
        // This function/slot should get called whenever the current model has been changed externally
        void currentModelChanged(bagel_gui::ModelInterface *model);
        // This function/slot should get called whenever we have edited some of the model data (in e.g. the fields)
        void updateModel();

        void setViewFilter(int v);
        void layoutsClicked(const QModelIndex &index);
        void addRemoveLayout();
        void openUrl(const QUrl &);
        void validateYamlSyntax();

    // TODO: Need a signal to notify others about changes in the fields. Used to update the current model values.

    private:
        std::map<QLabel *, QWidget *> widgets; // Generic list of label:
        bagel_gui::BagelGui *bagelGui;
        XRockGUI *xrockGui;
        bagel_gui::ModelInterface* currentModel = nullptr;

        QListWidget *layouts;
        configmaps::ConfigMap layoutMap;
        std::string currentLayout, edition;
        std::map<std::string, QCheckBox *> layoutCheckBoxes;
        QLineEdit *layoutName;
        QTextEdit *includes, *annotations, *interfaces;
        QLabel *dataStatusLabel;

        bool ignoreUpdate;
        void handleEditionLayout();
        void updateCurrentLayout();
    };

} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_MODEL_WIDGET_HPP
