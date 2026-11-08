#include "validate.h"
#include "struc.h"
#include "validation.h"
#include "wlogCommon.h"
#include <algorithm>
#include <qboxlayout.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qdialogbuttonbox.h>
#include <qformlayout.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qoverload.h>
#include <qpushbutton.h>
#include <qscrollarea.h>
#include <qspinbox.h>
#include <qwidget.h>

struct checkWidget{
    const wlog::LogFormat *format;
    const wlog::validationCheck *check;
    int column;
    QCheckBox *box;
    QDoubleSpinBox *spin;
};

static void addCheckRow(QFormLayout *form, std::vector<checkWidget> &out, const wlog::LogFormat *fmt, const wlog::validationCheck &chk, int column){
    auto *box = new QCheckBox(chk.title);
    box->setToolTip(chk.description);

    QDoubleSpinBox *spin = nullptr;
    if (chk.param == wlog::checkParam::number){
        spin = new QDoubleSpinBox();
        spin->setRange(-1e9, 1e9);

        if (chk.defaultParam && column >= 0){
            double def = chk.defaultParam(&fmt->columns[column]);
            if (!std::isnan(def)){
                spin->setValue(def);
            }
        }

        spin->setEnabled(true);
        QObject::connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), box, [box](){
            box->setChecked(true);
        });
        form->addRow(box, spin);
    }
    else {
        form->addRow(box);
    }
    out.push_back({fmt, &chk, column, box, spin});
}

static QGroupBox *buildFormatGroup(const wlog::LogFormat *fmt, std::vector<checkWidget> &out){
    QGroupBox *group = new QGroupBox(QString("V%1 records").arg(fmt->version));
    auto *layout = new QVBoxLayout(group);

    for (size_t col = 0; col < fmt->columnCount; col++){
        if (fmt->columns[col].format != wlog::displayFormat::number) {
            continue;
        }

        auto *sub = new QGroupBox(QString("%1 (%2)").arg(QString(fmt->columns[col].title), QString(fmt->columns[col].unit)));
        auto *form = new QFormLayout(sub);
        
        for (size_t i = 0; i < wlog::numberCheckCount; i++){
            addCheckRow(form, out, fmt, wlog::numberChecks[i], static_cast<int>(col));
        }
        layout->addWidget(sub);
    }

    std::set<std::string> seen;

    for (size_t i = 0; i < fmt->checkCount; i++){
        const wlog::validationCheck &check = fmt->checks[i];
        
        auto *sub = new QGroupBox(check.group);
        auto *form = new QFormLayout(sub);
        addCheckRow(form, out, fmt, check, -1);
        layout->addWidget(sub);
    }
    return group;
}

static std::vector<activeCheck> collect(const std::vector<checkWidget> &widgets){
    std::vector<activeCheck> active;
    for (const auto &w : widgets){
        if (w.box->isChecked()){
            active.push_back({w.format, w.check, w.column, w.spin ? w.spin->value() : 0.0});
        }
    }
    return active;
}

void drawValidationWindow(QWidget *parent, const std::vector<wlogFileInfo> &files){
    QDialog dialog(parent);
    dialog.setWindowTitle("Validation");
    dialog.resize(800, 600);

    QVBoxLayout *mainLayoutOfLies = new QVBoxLayout(&dialog);

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayoutOfLies->addLayout(mainLayout);

    QVBoxLayout *validationSelection = new QVBoxLayout();
    mainLayout->addLayout(validationSelection, 4); //4:6 split with affectedRows

    //validation options selection
    validationSelection->addWidget(new QLabel("Record Validation Options"));

    QScrollArea *selectionScrollArea = new QScrollArea();
    selectionScrollArea->setWidgetResizable(true);
    validationSelection->addWidget(selectionScrollArea);

    QWidget *selectionScrollAreaContent = new QWidget();
    QVBoxLayout *selectionContentLayout = new QVBoxLayout(selectionScrollAreaContent); 
    selectionScrollArea->setWidget(selectionScrollAreaContent);
    
    std::vector<const wlog::LogFormat*> openFormats;
    for (const auto &file : files){
        if (std::find(openFormats.begin(), openFormats.end(), file.format) == openFormats.end()){
            openFormats.push_back(file.format);
        }
    }

    selectionContentLayout->setSpacing(25);

    std::vector<checkWidget> selectionWidgets;
    for (const wlog::LogFormat *format : openFormats){
        selectionContentLayout->addWidget(buildFormatGroup(format, selectionWidgets));
    }
    selectionContentLayout->addStretch();

    //affected rows
    QVBoxLayout *affectedRows = new QVBoxLayout();
    mainLayout->addLayout(affectedRows, 6);

    //dialog buttons
    QDialogButtonBox *buttons = new QDialogButtonBox();

    QPushButton *validateButton = buttons->addButton("Validate", QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Close);

    QObject::connect(validateButton, &QPushButton::clicked, &dialog, [&](){
        const std::vector<activeCheck> activeChecks = collect(selectionWidgets);
        const std::vector<fileValidation> fileResults = validateFiles(files, activeChecks);

        for (const fileValidation &fileResult : fileResults){
            qDebug() << fileResult.file->path << fileResult.issues.size() << "issues";
        }
    });

    mainLayout->addStretch();
    mainLayoutOfLies->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}
