#include "BuildModuleDialog.hpp"
#include "XRockGUI.hpp"
#include "ComponentModelInterface.hpp"
#include <config_map_gui/DataWidget.h>
#include <mars_utils/misc.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QFrame>
#include <QMessageBox>
#include "utils/WaitCursorRAII.hpp"

using namespace configmaps;

namespace xrock_gui_model
{

    BuildModuleDialog::BuildModuleDialog(XRockGUI *xrockGui)
        : xrockGui(xrockGui)
    {
        setWindowTitle("Build Module Dialog");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        //  layouts
        QVBoxLayout *layout = new QVBoxLayout(this);
        QHBoxLayout *hlayout = new QHBoxLayout();

        QLabel *label = new QLabel("Module Name");
        moduleNameEdit = new QLineEdit();
        //add  widgets to layout
        hlayout->addWidget(label);
        hlayout->addWidget(moduleNameEdit);
        layout->addLayout(hlayout);
        //get the model info
        ComponentModelInterface *cm = dynamic_cast<ComponentModelInterface *>(xrockGui->getBagelGui()->getCurrentModel());
        ConfigMap map = cm->getModelInfo();
        uriToplvlcm = map["uri"].getString();
        // get unresolved abstract within the model network
        ConfigMap data;
        {
            WaitCursorRAII _;
            data = xrockGui->db->getUnresolvedAbstracts(uriToplvlcm);
        }
        if (data["unresolved_abstracts"].size() > 0)
        {
            QFrame *separator = new QFrame;
            separator->setFrameShape(QFrame::HLine);
            separator->setFrameShadow(QFrame::Sunken);

            layout->addWidget(separator);
            //add a header for user
            QLabel *header = new QLabel("Unresolved abstracts:");
            layout->addWidget(header);
            listWidget = new QListWidget(this);

            // Unresolved abstracts:
            for (auto& unresolved_abstract : data["unresolved_abstracts"])
            {
                std::string abstract_uri = unresolved_abstract["uri"];
                std::string alias = unresolved_abstract["alias"];
                std::string abstract_name = unresolved_abstract["name"];
                std::string abstract_version = unresolved_abstract["version"];
                QHBoxLayout *h = new QHBoxLayout();
                QWidget *widget = new QWidget();
                QLabel *display_abstract_name_label;
                if (alias != "")
                    display_abstract_name_label = new QLabel(QString::fromStdString(abstract_name) + " (" + QString::fromStdString(alias) + ")" + "(" + QString::fromStdString(abstract_version) + ")");
                else
                    display_abstract_name_label = new QLabel(QString::fromStdString(abstract_name) + " (" + QString::fromStdString(abstract_version) + ")");
                display_abstract_name_label->setProperty("uri", QString::fromStdString(abstract_uri));// tag the combobox element with the uri
                
                QComboBox *combox = new QComboBox();
                for (auto& implementation : unresolved_abstract["implementations"])
                {
                    QString displayText = QString::fromStdString(implementation["name"].getString() + " (" + implementation["version"].getString() + ")");
                    QString uri = QString::fromStdString(implementation["uri"].getString());
                    QVariant tag;
                    tag.setValue(uri); // tag the combobox element with the uri
                    combox->addItem(displayText, tag);
                }
                QListWidgetItem *item = new QListWidgetItem(listWidget);
                h->addWidget(display_abstract_name_label);
                h->addWidget(combox);
                widget->setLayout(h);
                item->setSizeHint(widget->minimumSizeHint());
                listWidget->addItem(item);
                listWidget->setItemWidget(item, widget);
                // save the references to the Qcombobox and Qlable so that we have it in the map
                selectedImplementationsWidgets[display_abstract_name_label] = combox;
            }
            layout->addWidget(listWidget);
        }

        // buttons
        listWidget = new QListWidget();
        button = new QPushButton("Build");
        connect(button, SIGNAL(clicked()), this, SLOT(onBuildButtonClicked()));
        layout->addWidget(button);

        setLayout(layout);
        adjustSize();
        setMinimumWidth(640);
        //setMinimumHeight(640);
    } 

    void BuildModuleDialog::onBuildButtonClicked()
    {
        // 1. validate fields (module name should not be empty..)
        if (moduleNameEdit->text().isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Module Name Field is empty! Please set a Name", QMessageBox::Ok);
            return;
        }
        // 2. collect resolved implementations from list
        std::map<std::string, std::string> selected_implementations;
        for (const auto &[label, combobox] : selectedImplementationsWidgets)
        {
            // Retrieve the QVariant data from the QLabel (optional)
            QString abstract_uri = label->property("uri").toString(); // get the tag of abstract label 
            QString impl_uri = combobox->itemData(combobox->currentIndex(), Qt::UserRole).toString(); // get the tag of the selected implementation
            
            selected_implementations[abstract_uri.toStdString()] = impl_uri.toStdString();
        }

        // 3. before we call db.buildModule(), we save the current model( if has changes)
        //TODO: hasChanges returns true even if there were no changes.
        if (xrockGui->getBagelGui()->getCurrentTabView()->hasChanges())
        {
            bool stored;
            {
                WaitCursorRAII _; 
                stored = xrockGui->storeComponentModel();
            }
            if (!stored)
            {
                //TODO: check history
                QMessageBox::critical(nullptr, "Error", "Could not store component model to database", QMessageBox::Ok);
                
                return;
            }
        }
        bool build;
        {
            WaitCursorRAII _; 
            build = xrockGui->db->buildModule(uriToplvlcm, moduleNameEdit->text().toStdString(), selected_implementations);
        }
        if (!build)
        {
            QMessageBox::critical(nullptr, "Error", "Could not build Module to database", QMessageBox::Ok);
            return;
        }
        QMessageBox::information(nullptr, "Success", "Module has been successfully saved into database", QMessageBox::Ok);
        close();
    }

    BuildModuleDialog::~BuildModuleDialog()
    {
        // Destructor implementation
        // delete comboBox;
    }

} // end of namespace xrock_gui_model
