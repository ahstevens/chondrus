#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <complex>

class Gabor
{
public:

	Gabor()
	{
	}

	~Gabor()
	{
	}

	float get(glm::vec2 pos)
	{
		return m_ComplexSinusoid.get(pos).real() * m_GaussianKernel.get(pos);
	}

	void setGaussianKernel(glm::vec2 center, glm::vec2 spread, float angle_rad, float amplitude)
	{
		m_GaussianKernel.setCenter(center);
		m_GaussianKernel.setSpread(spread);
		m_GaussianKernel.setAngle(angle_rad);
		m_GaussianKernel.setAmplitude(amplitude);
	}

	void setGaussianKernelCenter(glm::vec2 c) { m_GaussianKernel.setCenter(c); }
	glm::vec2 getGaussianKernelCenter() { return  m_GaussianKernel.getCenter(); }

	void setGaussianKernelSpread(glm::vec2 s) { m_GaussianKernel.setSpread(s); }
	glm::vec2 getGaussianKernelSpread() { return  m_GaussianKernel.getSpread(); }

	void setGaussianKernelAngle(float a) { m_GaussianKernel.setAngle(a); }
	float getGaussianKernelAngle() { return  m_GaussianKernel.getAngle(); }

	void setGaussianKernelAmplitude(float a) { m_GaussianKernel.setAmplitude(a); }
	float getGaussianKernelAmplitude() { return  m_GaussianKernel.getAmplitude(); }

	void setComplexSinusoid(float distance, float angleRad)
	{
		m_ComplexSinusoid.setDistance(distance);
		m_ComplexSinusoid.setAngle(angleRad);
	}

	void setComplexSinusoidDistance(float distance) { m_ComplexSinusoid.setDistance(distance); }
	float getComplexSinusoidDistance() { return m_ComplexSinusoid.getDistance(); }

	void setComplexSinusoidAngle(float angle) { m_ComplexSinusoid.setAngle(angle); }
	float getComplexSinusoidAngle() { return m_ComplexSinusoid.getAngle(); }

private:
	class GaussianKernel {
	public:
		GaussianKernel()
			: m_vec2Center(glm::vec2(0.f, 0.f))
			, m_vec2Spread(glm::vec2(1.f, 1.f))
			, m_fAngle(0.f)
			, m_fAmplitude(1.f)
			, m_bNeedsRefresh(true)
		{}

		void setCenter(glm::vec2 c) { this->m_vec2Center = c; }
		glm::vec2 getCenter() { return this->m_vec2Center; }

		void setSpread(glm::vec2 s) { this->m_vec2Spread = s; m_bNeedsRefresh = true; }
		glm::vec2 getSpread() { return this->m_vec2Spread; }

		void setAngle(float a) { this->m_fAngle = a; m_bNeedsRefresh = true; }
		float getAngle() { return this->m_fAngle; }

		void setAmplitude(float a) { this->m_fAmplitude = a; }
		float getAmplitude() { return this->m_fAmplitude; }

		float get(glm::vec2 pos)
		{
			if (m_bNeedsRefresh)
				calculateCoefficients();

			glm::vec2 d = pos - m_vec2Center;
			return m_fAmplitude * exp(	-( _a * d.x * d.x - 2.f * _b * d.x * d.y + _c * d.y * d.y ) );
		}

	private:
		glm::vec2 m_vec2Center;
		glm::vec2 m_vec2Spread;
		float m_fAngle;
		float m_fAmplitude;
		float _a, _b, _c;
		bool m_bNeedsRefresh;

		void calculateCoefficients()
		{
			float sXsq = m_vec2Spread.x * m_vec2Spread.x;
			float sYsq = m_vec2Spread.y * m_vec2Spread.y;

			this->_a =  0.5f  * (pow(cos(this->m_fAngle), 2) / sXsq) + 0.5f  * (pow(sin(this->m_fAngle), 2) / sYsq);
			this->_b = -0.25f * (sin(2 * this->m_fAngle)     / sXsq) + 0.25f * (sin(2 * this->m_fAngle)     / sYsq);
			this->_c =  0.5f  * (pow(sin(this->m_fAngle), 2) / sXsq) + 0.5f  * (pow(cos(this->m_fAngle), 2) / sYsq);

			m_bNeedsRefresh = false;
		}

	};

	class ComplexSinusoid {
	public:
		ComplexSinusoid()
			: m_fDistance(0.f)
			, m_fAngle(0.f)
			, m_bNeedsRefresh(true)
		{}

		void setDistance(float d) 
		{ 
			m_fDistance = d; 
			m_bNeedsRefresh = true;
		}
		float getDistance() { return m_fDistance; }

		void setAngle(float a)
		{
			m_fAngle = a;
			m_bNeedsRefresh = true;
		}
		float getAngle() { return m_fAngle; }

		std::complex<float> get(glm::vec2 pos)
		{
			if (m_bNeedsRefresh)
				calculateSpatialCentralFrequency();

			return exp(glm::two_pi<float>()           // 2pi
				* std::complex<float>(0.f, 1.f)       // 0 + 1i
				* glm::dot(m_vec2SpatialCentralFrequency, pos));
		}

	private:
		float m_fDistance;
		float m_fAngle;
		glm::vec2 m_vec2SpatialCentralFrequency;
		bool m_bNeedsRefresh;

		void calculateSpatialCentralFrequency()
		{
			//m_vec2SpatialCentralFrequency = glm::vec2(1.f / m_fDistance * sin(m_fAngle), 1.f / m_fDistance * cos(m_fAngle));
			m_vec2SpatialCentralFrequency = glm::vec2(m_fDistance * sin(m_fAngle), m_fDistance * cos(m_fAngle));
			m_bNeedsRefresh = false;
		}

	};

	GaussianKernel m_GaussianKernel;
	ComplexSinusoid m_ComplexSinusoid;
};

