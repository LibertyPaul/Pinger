#ifndef COLORGENERATOR_HPP
#define COLORGENERATOR_HPP

#include <QColor>
#include <random>

class ColorGenerator{
	std::default_random_engine re;
	std::uniform_real_distribution<double> unif;

	double lastColor;
	static constexpr double goldenRatio = 0.618033988749895;

public:
	ColorGenerator();
	~ColorGenerator();

	QColor generate();
};

#endif // COLORGENERATOR_HPP
