#include "widgets/ModulationControl.hpp"
#include "app/Theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>

ModulationControl::ModulationControl(int componentId, ParameterType p, QWidget* parent):
    QWidget(parent),
    componentId_(componentId),
    parameter_(p),
    paramLabel_(new QLabel(this)),
    depthSlider_(new SliderWidget(ParameterType::DEPTH, this)),
    strategyLabel_(new QLabel(this)),
    strategySelector_(new QComboBox(this)),
    modIndicator_(new ModulationIndicator(this))
{
    QString pString = QString::fromStdString(GET_PARAMETER_TRAIT_MEMBER(p, name));
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
            emit modulationDepthEdited(
                componentId_, 
                parameter_, 
                std::get<GET_PARAMETER_VALUE_TYPE(ParameterType::DEPTH)>(depthSlider_->getValue()) 
            );
        }
    );

    connect( 
        strategySelector_, &QComboBox::currentIndexChanged,
        this, [this](){
            emit modulationStrategyEdited(
                componentId_, 
                parameter_, 
                static_cast<ModulationStrategy>(strategySelector_->currentData().toInt())
            );
        }
    );

}

void ModulationControl::setConnectionStatus(bool active){
    modIndicator_->setActive(active);
}

void ModulationControl::setupLayout(){
    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* header = new QHBoxLayout();
    QHBoxLayout* body = new QHBoxLayout();

    header->addWidget(paramLabel_);
    header->addStretch();
    header->addWidget(modIndicator_);
    layout->addLayout(header);

    body->addWidget(depthSlider_);
    QVBoxLayout* strategy = new QVBoxLayout();
    strategy->addWidget(strategyLabel_);
    strategy->addWidget(strategySelector_);
    body->addLayout(strategy);
    layout->addLayout(body);
}

void ModulationControl::onModelDepthChanged(int componentId, ParameterType p, double depth){
    if ( componentId_ != componentId || p != parameter_ ) return ;
    depthSlider_->setValue(depth, true);
}

void ModulationControl::onModelStrategyChanged(int componentId, ParameterType p, ModulationStrategy strategy){
    if ( componentId_ != componentId || p != parameter_ ) return ;

    auto idx = strategySelector_->findData(static_cast<uint8_t>(strategy));
    strategySelector_->setCurrentIndex(idx);}

void ModulationControl::onModelConnectionUpdated(int componentId, ParameterType p, bool active){
    if ( componentId_ != componentId || p != parameter_ ) return ;

    modIndicator_->setActive(active);
}
