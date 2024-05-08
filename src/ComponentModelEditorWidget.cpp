#include "ComponentModelEditorWidget.hpp"
#include "LinkHardwareSoftwareDialog.hpp"
#include "XRockGUI.hpp"
#include "ConfigureDialog.hpp"
#include "ConfigMapHelper.hpp"
#include "ImportDialog.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QRegExp>
#include <QTextEdit>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>
#include <bagel_gui/BagelGui.hpp>
#include <bagel_gui/BagelModel.hpp>
#include <mars/utils/misc.h>
#include <QDesktopServices>

using namespace configmaps;

namespace xrock_gui_model
{

    ComponentModelEditorWidget::ComponentModelEditorWidget(mars::cfg_manager::CFGManagerInterface *cfg,
                             bagel_gui::BagelGui *bagelGui, XRockGUI *xrockGui,
                             QWidget *parent) : mars::main_gui::BaseWidget(parent, cfg, "ComponentModelEditorWidget"), bagelGui(bagelGui),
                                                xrockGui(xrockGui)
    {
        try
        {
            QGridLayout *layout = new QGridLayout();
            QVBoxLayout *vLayout = new QVBoxLayout();
            size_t i = 0;
            QLabel* lbl = new QLabel("uri");
            layout->addWidget(lbl, i, 0);
            uri = new QLineEdit();
            uri->setReadOnly(true);
            //connect(interfaces, SIGNAL(textChanged()), this, SLOT(updateModel()));
            layout->addWidget(uri, i++, 1);


           
            ConfigMap props = xrockGui->db->getPropertiesOfComponentModel();
            for(auto it: props)
            {
               std::string key = it.first;
                ConfigMap prop = it.second;
                QLabel *label = new QLabel(it.first.c_str());
                layout->addWidget(label, i, 0);
                if(prop["type"] =="array")
                {
                    // if property has some allowed values, its a combobox
                    QComboBox *combobox = new QComboBox();
                    for(auto allowed: prop["allowed_values"])
                    {
                        combobox->addItem(QString::fromStdString(allowed));
                    }
                    layout->addWidget(combobox, i++, 1);
                    connect(combobox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(updateModel()));
                    widgets[label] = combobox;
                }
                else if(prop["type"] =="string")
                {
                    // if property has no allowed values, its a qlineedit
                    QLineEdit *linedit = new QLineEdit();
                    layout->addWidget(linedit, i++, 1);
                    connect(linedit, SIGNAL(textChanged(const QString &)), this, SLOT(updateModel()));
                    widgets[label] = linedit;
                }
                else if(prop["type"] =="boolean")
                {
                    // if property has no allowed values, its a QCheckBox
                    QCheckBox *checkbox = new QCheckBox();
                    layout->addWidget(checkbox, i++, 1);
                    if(key == "can_have_parts")
                    {
                        checkbox->setEnabled(false);
                        checkbox->setChecked(true);
                    }
                    connect(checkbox,  SIGNAL(clicked(bool)), this, SLOT(updateModel()));
                    widgets[label] = checkbox;

                }
            }
            QLabel* l = new QLabel("types");
            layout->addWidget(l, i, 0);
            types = new QListWidget();
            layout->addWidget(types, i++, 1);
            QPushButton* minusBtn =new QPushButton("-"); 
            connect(minusBtn,  SIGNAL(clicked(bool)), this, SLOT(removeSelectedType()));

            QPushButton* plusBtn =new QPushButton("+"); 
            connect(plusBtn,  SIGNAL(clicked(bool)), this, SLOT(addType()));
 
            QHBoxLayout* hbox  = new QHBoxLayout();
            hbox->addWidget(minusBtn);
            hbox->addWidget(plusBtn);
            layout->addLayout(hbox, i++, 1);

            // Special property landing in the infamous 'data' property
            l = new QLabel("annotations");
            layout->addWidget(l, i, 0);
            annotations = new QTextEdit();
            layout->addWidget(annotations, i++, 1);
            dataStatusLabel = new QLabel();
            dataStatusLabel->setText("valid Yaml syntax");
            dataStatusLabel->setAlignment(Qt::AlignCenter);
            if (annotations)
            {
                dataStatusLabel->setStyleSheet("QLabel { background-color: #128260; color: white; }");
                layout->addWidget(dataStatusLabel, i++, 1);
                connect(annotations, SIGNAL(textChanged()), this, SLOT(validateYamlSyntax()));
            }


            // Finalize layout
            vLayout->addLayout(layout);
            vLayout->addStretch();
            QGridLayout *gridLayout = new QGridLayout();
            l = new QLabel("Layout:");
            gridLayout->addWidget(l, 0, 0);
            i = 0;
            if(props.hasKey("domain") && props["domain"].hasKey("allowed_values"))
            {
                for (auto allowed : props["domain"]["allowed_values"])
                {
                    QCheckBox *check = new QCheckBox(QString::fromStdString(allowed));
                    check->setChecked(true);
                    connect(check, SIGNAL(stateChanged(int)), this, SLOT(setViewFilter(int)));
                    gridLayout->addWidget(check, i / 2, i % 2 + 1);
                    layoutCheckBoxes[allowed] = check;
                    i++;
                }
            }
            vLayout->addLayout(gridLayout);
            layouts = new QListWidget();
            vLayout->addWidget(layouts);
            connect(layouts, SIGNAL(clicked(const QModelIndex &)),
                    this, SLOT(layoutsClicked(const QModelIndex &)));
            layouts->addItem("overview");
            QHBoxLayout *hLayout = new QHBoxLayout();
            layoutName = new QLineEdit("new layout");
            hLayout->addWidget(layoutName);
            QPushButton *b = new QPushButton("add/remove");
            connect(b, SIGNAL(clicked()), this, SLOT(addRemoveLayout()));
            hLayout->addWidget(b);
            vLayout->addLayout(hLayout);
            hardwareLinkBtn = new QPushButton("Manage Hardware Links");
            hardwareLinkBtn->setVisible(false);
            connect(hardwareLinkBtn, SIGNAL(clicked(bool)), this, SLOT(linkHardware()));
            vLayout->addWidget(hardwareLinkBtn);
            setLayout(vLayout);
            this->clear();
        }
        catch (const std::exception &e)
        {
            std::stringstream ss;
            ss << "Exception thrown: " << e.what() << "\tAt " << __FILE__ << ':' << __LINE__ << '\n'
               << "\tAt " << __PRETTY_FUNCTION__ << '\n';
            QMessageBox::warning(nullptr, "Warning", QString::fromStdString(ss.str()), QMessageBox::Ok);
        }
    }

    ComponentModelEditorWidget::~ComponentModelEditorWidget(void)
    {
        // Cleanup widgets
        for(auto& [label, widget] : widgets)
        {
            delete label;
            delete widget;
        }
    }

    void ComponentModelEditorWidget::deinit(void)
    {
        // 20221107 MS: Why does this widget set a model path?
        //cfg->setPropertyValue("XRockGUI", "modelPath", "value", modelPath);
    }

    void ComponentModelEditorWidget::update_widgets(configmaps::ConfigMap& info)
    {
        // Lets iterate over model's properties
        auto is_property_key = [&](const std::string& key) -> bool
        {
            ConfigMap props = xrockGui->db->getPropertiesOfComponentModel();
            for (auto it: props)
            {
                if(key == it.first)
                {
                    return true;
                }
            }
            return false;
        };
        for (auto it = info.begin(); it != info.end(); ++it)
        {
            std::string key = it->first;
            ConfigItem value = it->second;
            if(is_property_key(key))
            {
                this->update_prop_widget(key,  value);
            }

        }

        for (auto it = info["versions"][0].beginMap(); it != info["versions"][0].endMap(); ++it)
        {
            std::string key = it->first;
            ConfigItem value = it->second;

            if(key == "name")
            {
                key = "version"; // maybe just rename it so when we pass it to update_prop_widget it will update the version widget
            }


            if(not is_property_key(key) or !value.isAtom())
            {
                continue; // skip interfaces .. non-properties
            }
            this->update_prop_widget(key,  value);
        }
        //if(info["versions"][0].hasKey("interfaces"))
        //{
        //    std::string inter = info["versions"][0]["interfaces"].toYamlString().c_str();
        //    interfaces->setText(QString::fromStdString(inter));
        //}
        if(info["versions"][0].hasKey("data"))
        {
            // filter gui annotations since they are handled by the bage_gui itself
            ConfigMap data = info["versions"][0]["data"];
            if(data.hasKey("gui"))
            {
                data.erase("gui");
            }
            std::string dataString = data.toYamlString().c_str();
            annotations->setText(QString::fromStdString(dataString));

            // update layout list (part of data field)
            if(info["versions"][0]["data"].hasKey("gui"))
            {
                if(info["versions"][0]["data"]["gui"].hasKey("layouts"))
                {
                    std::string defLayout = info["versions"][0]["data"]["gui"]["defaultLayout"];
                    for(auto it: (ConfigMap)info["versions"][0]["data"]["gui"]["layouts"])
                    {
                        layouts->addItem(it.first.c_str());
                        if(it.first == defLayout)
                        {
                            layouts->setCurrentItem(layouts->item(layouts->count()-1));
                        }
                    }
                }
            }
        }


    }

    void ComponentModelEditorWidget::currentModelChanged(bagel_gui::ModelInterface *model)
    {
        ComponentModelInterface* newModel = dynamic_cast<ComponentModelInterface *>(model);
        if (!newModel) return;
        fprintf(stderr, "getModelInfo\n");
        auto info = newModel->getModelInfo();
        for(auto &[k, v]: (ConfigMap)info)
        {
            fprintf(stderr, "%s:", k.c_str());
            fprintf(stderr, " %s\n", v.toYamlString().c_str());
        }
        currentModel = nullptr;
        fprintf(stderr, "update widgets\n");
        this->update_widgets(info);
        // set uri info to uri text field 
        fprintf(stderr, "output info\n");
        if(info.hasKey("uri"))
        {
          uri->setText(QString::fromStdString(info["uri"]));
        }
        // FILL TYPES list
        for(auto type : info["types"])
        {
            std::string name = type["name"];
            std::string version = type["version"];
            QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(name + " " + version));
            QVariant tag; 
            tag.setValue(QString::fromStdString(type.toJsonString()));
            item->setData(Qt::UserRole, tag);
            types->addItem(item);
        } 
        currentModel = newModel;
        updateModel();
        updateManageHardwareLinkButtonState();
        fprintf(stderr, "done\n");
    }

    void ComponentModelEditorWidget::updateManageHardwareLinkButtonState()
    {
        bool enableHardwareLinkButton = false;
        if (currentModel)
        {
            configmaps::ConfigMap basicModel = currentModel->getModelInfo();
            std::string domain = basicModel["domain"];
            if (domain == "SOFTWARE")
                enableHardwareLinkButton = true;
        }
        hardwareLinkBtn->setVisible(enableHardwareLinkButton);
    }
    
    void ComponentModelEditorWidget::update_prop_widget(const std::string &prop_name, configmaps::ConfigAtom &value)
    {
        for (auto &[label, widget] : widgets)
        {
            if (label->text().toStdString() == prop_name)
            {
                if(QLineEdit * le = dynamic_cast<QLineEdit *>(widget))
                {
                    le->setText(QString::fromStdString((std::string)value));
                    break;
                }
                else if(QComboBox * cb = dynamic_cast<QComboBox *>(widget))
                {

                    cb->setCurrentIndex(cb->findData(QString::fromStdString((std::string)value), Qt::DisplayRole)); // <- refers to the item text
                    break;
                }
                else if(QCheckBox * ckb = dynamic_cast<QCheckBox *>(widget))
                {
                    ckb->setChecked((bool)value);
                    break;
                }
                
            }
        }
    }

    bool ComponentModelEditorWidget::has_prop_widget(const std::string& prop_name)
    {
        for (auto &[label, widget] : widgets)
        {
            if (label->text().toStdString() == prop_name)
                return true;
        }
        return false;
    }

    configmaps::ConfigAtom ComponentModelEditorWidget::get_prop_widget_value(const std::string &prop_name)
    {
        for (auto &[label, widget] : widgets)
        {
            if (label->text().toStdString() == prop_name)
            {
                if (QComboBox *cb = dynamic_cast<QComboBox *>(widget))
                    return ConfigAtom(cb->currentText().toStdString());
                else if(QLineEdit* le= dynamic_cast<QLineEdit *>(widget))
                    return ConfigAtom(le->text().toStdString());
                else if(QCheckBox* cbx = dynamic_cast<QCheckBox *>(widget))
                    return ConfigAtom(cbx->isChecked());
            }
        }
        throw std::runtime_error("no prop found with name " + prop_name);
    }
    void ComponentModelEditorWidget::updateModel()
    {
        if (!currentModel) return;
        ConfigMap updatedMap(currentModel->getModelInfo());
        // Update toplvl properties
        for (auto &[key, value] : updatedMap)
        {
            if (!has_prop_widget(key))
                continue;
            value = get_prop_widget_value(key);
        }
        // Update 'second' level properties (all due to having the basic model legacy :/)
        ConfigMap& secondLevel(updatedMap["versions"][0]);
        for (auto &it : secondLevel)
        {
            // NOTE: The 'name' key on the second level is tied to the 'version' widget
            std::string key = it.first;
            if (key == "name")
                key = "version";
            if (!has_prop_widget(key))
                continue;
            it.second = get_prop_widget_value(key);
        }
        // Special property 'data'
        ConfigMap annoMap = ConfigMap::fromYamlString(annotations->toPlainText().toStdString());
        if(updatedMap["versions"][0].hasKey("data"))
        {
            updatedMap["versions"][0]["data"].updateMap(annoMap);
        }
        else
        {
            updatedMap["versions"][0]["data"] = annoMap;
        }
        // Sync types list with basic model
        updatedMap["types"] = ConfigVector();
        for(int i = 0; i < types->count(); ++i)
        {
            QListWidgetItem* item = types->item(i);
            ConfigMap type = ConfigMap::fromJsonString(item->data(Qt::UserRole).value<QString>().toStdString());
            updatedMap["types"].push_back(type);
        }
        currentModel->setModelInfo(updatedMap);
    }

    void ComponentModelEditorWidget::setViewFilter(int v)
    {
        for (const auto& [label, checkbox] : layoutCheckBoxes)
        {
            bagelGui->setViewFilter(label, checkbox->isChecked());
        }
    }

    void ComponentModelEditorWidget::layoutsClicked(const QModelIndex &index)
    {

        try
        {
            QVariant v = layouts->model()->data(index, 0);
            if (v.isValid())
            {
                std::string layout = v.toString().toStdString();
                dynamic_cast<ComponentModelInterface *>(currentModel)->selectLayout(layout);
            }
        }
        catch (const std::exception &e)
        {
            std::stringstream ss;
            ss << "Exception thrown: " << e.what() << "\tAt " << __FILE__ << ':' << __LINE__ << '\n'
               << "\tAt " << __PRETTY_FUNCTION__ << '\n';
            QMessageBox::warning(nullptr, "Warning", QString::fromStdString(ss.str()), QMessageBox::Ok);
        }
    }

    void ComponentModelEditorWidget::addRemoveLayout()
    {
        try
        {
            std::string name = layoutName->text().toStdString();
            for (int i = 0; i < layouts->count(); ++i)
            {
                QVariant v = layouts->item(i)->data(0);
                if (v.isValid())
                {
                    std::string layout = v.toString().toStdString();
                    if (layout == name)
                    {
                        QListWidgetItem *item = layouts->item(i);
                        delete item;
                        dynamic_cast<ComponentModelInterface *>(currentModel)->removeLayout(layout);
                        if (layouts->count() > 0)
                        {
                            layouts->setCurrentItem(layouts->item(0));
                            std::string currentLayout = layouts->item(0)->data(0).toString().toStdString();
                            layoutName->setText(currentLayout.c_str());
                            dynamic_cast<ComponentModelInterface *>(currentModel)->selectLayout(currentLayout);
                        }
                        else
                        {
                            layouts->setCurrentItem(0);
                        }
                        // we deleted the layout and return
                        return;
                    }
                }
            }
            // if we get here we didn't find the layout and add a new one
            layouts->addItem(name.c_str());
            layouts->setCurrentItem(layouts->item(layouts->count() - 1));
            dynamic_cast<ComponentModelInterface *>(currentModel)->addLayout(name);
        }

        catch (const std::exception &e)
        {
            std::stringstream ss;
            ss << "Exception thrown: " << e.what() << "\tAt " << __FILE__ << ':' << __LINE__ << '\n'
               << "\tAt " << __PRETTY_FUNCTION__ << '\n';
            QMessageBox::warning(nullptr, "Warning", QString::fromStdString(ss.str()), QMessageBox::Ok);
        }
    }

    void ComponentModelEditorWidget::clear()
    {
        // NOTE: Before clearing the fields, we have to set currentModel to null to prevent updateModel() to be triggered
        currentModel = nullptr;
        for (auto &[label, widget] : widgets)
        {
            if (QLineEdit *field = dynamic_cast<QLineEdit *>(widget))
            {
                field->clear();
            }
        }
        annotations->clear();
        //interfaces->clear();
        layouts->clear();
        uri->clear();
        types->clear();
    }


    void ComponentModelEditorWidget::openUrl(const QUrl &link)
    {
        QDesktopServices::openUrl(link);
    }

    void ComponentModelEditorWidget::validateYamlSyntax()
    {
        const std::string data_text = annotations->toPlainText().toStdString();
        if (data_text.empty())
            return;
        try
        {
            ConfigMap tmpMap = ConfigMap::fromYamlString(data_text);
            dataStatusLabel->setText("valid Yaml syntax");
            dataStatusLabel->setStyleSheet("QLabel { background-color: #128260; color: white;}");
        }
        catch (...)
        {
            dataStatusLabel->setText("invalid Yaml syntax");
            dataStatusLabel->setStyleSheet("QLabel { background-color: red; color: white;}");
            return;
        }
        // If the content is valid, we can call updateModel()
        updateModel();
    }

    void ComponentModelEditorWidget::removeSelectedType()
    {
        QListWidgetItem *item = types->currentItem();
        if(item)
        {
            delete item;
            updateModel();
        }
    }

    void ComponentModelEditorWidget::addType(){
        // Pick cm from db 
        ImportDialog id(xrockGui, ImportDialog::Intention::ADD_TYPE);
        id.exec();
    
        auto model  = id.getModel();
        ConfigMap type;
        type["name"] = model["name"];
        type["version"] = model["versions"][0]["name"];
        std::string name = type["name"];
        std::string version = type["version"];

        ConfigMap currentMap(currentModel->getModelInfo());
        if(std::any_of(currentMap["types"].begin(), currentMap["types"].end(), 
        [&name, &version](auto& type){ return type["name"] == name && type["version"] == version;})){
            QMessageBox::warning(nullptr, "Warning", QString::fromStdString("Type " + name + " is already added!"), QMessageBox::Ok);
        } else {
            QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(name + " " + version));
            QVariant tag; 
            tag.setValue(QString::fromStdString(type.toJsonString()));
            item->setData(Qt::UserRole, tag);
            types->addItem(item);

            updateModel(); 
        }
    }
    void ComponentModelEditorWidget::linkHardware()
    {
        if(currentModel) {
            auto currentModelMap = currentModel->getModelInfo();
            LinkHardwareSoftwareDialog dialog(xrockGui, currentModelMap);
            dialog.exec();
            currentModel->setModelInfo(dialog.getSoftwareMap());
        }
     
    }
    
    

} // end of namespace xrock_gui_model
