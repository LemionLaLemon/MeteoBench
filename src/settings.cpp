#include "settings.h"
#include <qboxlayout.h>
#include <qcolor.h>
#include <qcolordialog.h>
#include <qdialogbuttonbox.h>
#include <qfont.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qobject.h>
#include <qpaintdevice.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>

QLabel *addNote(QBoxLayout *layout, const QString &text){
    auto *note = new QLabel(text);
    note->setWordWrap(true);
    note->setEnabled(false);
    layout->addWidget(note);
    return note;
}

void setButtonColor(QPushButton *btn, const QColor &c){
    QPixmap pm(50, 20);
    pm.fill(c);
    btn->setIcon(QIcon(pm));
    btn->setIconSize(pm.size());
}

void drawSettingsWindow(QWidget *parent, QSettings *settings){
    QDialog dialog(parent);
    dialog.setWindowTitle("Preferences");
    dialog.resize(800, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    QGroupBox *localeGroup = new QGroupBox("Locale");
    mainLayout->addWidget(localeGroup);

    //locale selection
    QVBoxLayout *localeLayout = new QVBoxLayout(localeGroup);

    //temperature format
    QHBoxLayout *temperatureSelect = new QHBoxLayout();
    temperatureSelect->addWidget(new QLabel("Temperature Format of Data"));
    temperatureSelect->addSpacing(50);
    localeLayout->addLayout(temperatureSelect);
    
    QRadioButton *celsiusRadio = new QRadioButton("Celsius");
    temperatureSelect->addWidget(celsiusRadio);
    celsiusRadio->setCheckable(true);
    celsiusRadio->setChecked(settings->value("locale/temperature", "celsius").toString() == "celsius");

    QRadioButton *fahrenheitRadio = new QRadioButton("Fahrenheit");
    temperatureSelect->addWidget(fahrenheitRadio);
    fahrenheitRadio->setCheckable(true);
    fahrenheitRadio->setChecked(settings->value("locale/temperature", "celsius").toString() != "celsius");
    temperatureSelect->addStretch();

    addNote(localeLayout, "This only changes the label and doesn't convert the data from DEG C to DEG F!");
    addNote(localeLayout, "To change your records from DEG C to DEG F or vice versa, use the Analysis tab.");
    addNote(localeLayout, "This is in case your station happens to be horribly broken!");

    QGroupBox *appearanceGroup = new QGroupBox("Appearance");
    mainLayout->addWidget(appearanceGroup);

    //apperance selection
    QVBoxLayout *appearanceLayout = new QVBoxLayout(appearanceGroup);
    
    //selected color
    QHBoxLayout *selectedColorSelect = new QHBoxLayout();
    selectedColorSelect->addWidget(new QLabel("Selection Color"));
    selectedColorSelect->addSpacing(50);
    appearanceLayout->addLayout(selectedColorSelect);

    QPushButton *selectColor = new QPushButton();
    selectColor->setEnabled(false);
    selectColor->setFixedSize(60, 30);
    selectedColorSelect->addWidget(selectColor);

    QColor selected = settings->value("appearance/selectedColor", "#316ac5").value<QColor>();
    setButtonColor(selectColor, selected);

    QObject::connect(selectColor, &QPushButton::clicked, &dialog, [&, selectColor](){
        QColor c = QColorDialog::getColor(selected, &dialog, "Select color");
        if (c.isValid()){
            selected = c;
            setButtonColor(selectColor, c);
        }
    });

    selectedColorSelect->addStretch();

    //tie in later (custom selected color selection)
    //QColorDialog *selectedColorSelection = new QColorDialog();
    //selectedColorSelect->addWidget(selectedColorSelection);

    mainLayout->addStretch();

    auto save = [&]() {
        settings->setValue("locale/temperature", celsiusRadio->isChecked() ? "celsius" : "fahrenheit");
        settings->setValue("appearance/selectedColor", selected.name());
    };

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&](){
        save();
        dialog.accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, &dialog, save);

    dialog.exec();
}