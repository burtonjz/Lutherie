#include "widgets/ModulationControl.hpp"
#include "app/Theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>

ModulationControl::ModulationControl(ParameterType p, QWidget* parent):
    QWidget(parent),
    parameter_(p),
    paramLabel_(new QLabel(this)),
    depthSlider_(new SliderWidget(ParameterType::DEPTH, this)),
    strategyLabel_(new QLabel(this)),
    strategySelector_(new QComboBox(this)),
    modIndicator_(new ModulationIndicator(this))
{
    QString pString = QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)));
    paramLabel_->setText(pString.toCaseFolded());
    paramLabel_->setStyleSheet(Theme::getLabelHeaderStyle());
    QFont f = paramLabel_->font();
    f.setCapitalization(QFont::AllUppercase);
    paramLabel_->setFont(f);

    strategyLabel_->setText("Modulation Strategy");
    #define X(NAME) \
        strategySelector_->addItem(QString::fromStdString( \
            modStrategyToString(ModulationStrategy::NAME)), static_cast<uint8_t>(ModulationStrategy::NAME));
    MODULATION_STRATEGY_LIST
    #undef X
    ModulationStrategy s = GET_PARAMETER_MODULATION_STRATEGY(p, ModulatorRange::UNKNOWN);
    auto idx = strategySelector_->findData(static_cast<uint8_t>(s));
    strategySelector_->setCurrentIndex(idx);

    setupLayout();

    connect( 
        depthSlider_, &ParameterWidget::valueChanged,
        this, [this](){
            emit depthEdited();
        }
    );

    connect( 
        strategySelector_, &QComboBox::currentIndexChanged,
        this, [this](){
            emit strategyEdited();
        }
    );

}

ParameterType ModulationControl::getType() const {
    return parameter_ ;
}

double ModulationControl::getDepth() const {
    return std::get<GET_PARAMETER_VALUE_TYPE(ParameterType::DEPTH)>(depthSlider_->getValue());
}

void ModulationControl::setDepth(double depth, bool block){
    depthSlider_->setValue(depth, block);
}

ModulationStrategy ModulationControl::getStrategy() const {
    return static_cast<ModulationStrategy>(strategySelector_->currentData().toInt());
}

void ModulationControl::setStrategy(ModulationStrategy strategy, bool block){
    QSignalBlocker blocker(strategySelector_);
    if ( !block ) blocker.unblock();

    auto idx = strategySelector_->findData(static_cast<uint8_t>(strategy));

    if ( idx == -1 ){
        qWarning() << "modulation strategy set to an invalid value. This is a programming bug." ;
        return ;
    }

    strategySelector_->setCurrentIndex(idx);
}

void ModulationControl::setConnectionStatus(bool active){
    modIndicator_->setActive(active);
}

void ModulationControl::setupLayout(){
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(Theme::PARAMETER_WIDGET_SPACING);

    QHBoxLayout* header = new QHBoxLayout();
    header->addWidget(paramLabel_);
    header->addStretch();
    header->addWidget(modIndicator_);
    layout->addLayout(header);

    QHBoxLayout* body = new QHBoxLayout();
    body->setAlignment(Qt::AlignTop);
    body->addWidget(depthSlider_, 0, Qt::AlignTop);

    QVBoxLayout* strategy = new QVBoxLayout();
    strategy->setContentsMargins(0,0,0,0);
    strategy->addWidget(strategyLabel_);
    strategy->addWidget(strategySelector_);
    strategy->addStretch();
    body->addLayout(strategy);

    layout->addLayout(body);
    layout->addStretch();
}

void ModulationControl::onModelDepthChanged(ParameterType p, double depth){
    if ( p != parameter_ ) return ;
    setDepth(depth, true);
}

void ModulationControl::onModelStrategyChanged(ParameterType p, ModulationStrategy strategy){
    if ( p != parameter_ ) return ;
    setStrategy(strategy, true);
}

