#include "colorgenerator.hpp"
#include <chrono>

ColorGenerator::ColorGenerator():
	re(std::chrono::system_clock::now().time_since_epoch().count()),
	unif(0, 1),
	lastColor(unif(re)){

}

ColorGenerator::~ColorGenerator(){

}


QColor ColorGenerator::generate(){
	this->lastColor = std::fmod(this->lastColor + this->goldenRatio, 1);
	QColor result = QColor::fromHsvF(this->lastColor, 0.5, 0.95, 0.95);

	return result;
}
