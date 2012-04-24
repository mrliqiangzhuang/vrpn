/** @file
	@brief Header

	@date 2012

	@author
	Jan Ciger
	<jan.ciger@reviatech.com>

*/

//           Copyright Reviatech 2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef INCLUDED_vrpn_OneEuroFilter_h_GUID_C56A0525_3809_44B7_AA16_98711638E762
#define INCLUDED_vrpn_OneEuroFilter_h_GUID_C56A0525_3809_44B7_AA16_98711638E762


// Internal Includes
#include <quat.h>

// Library/third-party includes
// - none

// Standard includes
// - none


// "One Euro" filter for reducing jitter
// http://hal.inria.fr/hal-00670496/

template<int DIMENSION = 3, typename Scalar = vrpn_float64>
class LowPassFilter {
	public:
		typedef Scalar scalar_type;
		typedef Scalar value_type[DIMENSION];
		typedef const scalar_type * value_return_type;
		LowPassFilter() : _firstTime(true) {
		}

		value_return_type filter(const value_type x, scalar_type alpha) {
			if (_firstTime) {
				_firstTime = false;
				memcpy(_hatxprev, x, sizeof(_hatxprev));
			}

			value_type hatx;
			for (int i = 0; i < DIMENSION; ++i) {
				hatx[i] = alpha * x[i] + (1 - alpha) * _hatxprev[i];
			}

			memcpy(_hatxprev, hatx, sizeof(_hatxprev));
			return _hatxprev;
		}

		value_return_type hatxprev() {
			return _hatxprev;
		}

	private:
		bool _firstTime;
		value_type _hatxprev;
};

typedef LowPassFilter<> LowPassFilterVec;

template<int DIMENSION = 3, typename Scalar = vrpn_float64>
class VectorFilterable {
	public:
		typedef	Scalar scalar_type;
		typedef Scalar value_type[DIMENSION];
		typedef value_type derivative_value_type;
		typedef Scalar * value_ptr_type;
		typedef value_type * derivative_value_ptr_type;
		typedef LowPassFilter<DIMENSION, Scalar> value_filter_type;
		typedef LowPassFilter<DIMENSION, Scalar> derivative_filter_type;
		typedef typename LowPassFilter<DIMENSION, Scalar>::value_return_type value_return_type;

		static void setDxIdentity(value_ptr_type dx) {
			for (int i = 0; i < DIMENSION; ++i) {
				dx[i] = 0;
			}
		}

		static void computeDerivative(derivative_value_type dx, value_return_type prev, const value_type current, scalar_type dt) {
			for (int i = 0; i < DIMENSION; ++i) {
				dx[i] = (current[i] - prev[i]) / dt;
			}
		}
		static scalar_type computeDerivativeMagnitude(derivative_value_type const dx) {
			scalar_type sqnorm = 0;
			for (int i = 0; i < DIMENSION; ++i) {
				sqnorm += dx[i] * dx[i];
			}
			return sqrt(sqnorm);
		}

};
template<typename Filterable = VectorFilterable<> >
class OneEuroFilter {
	public:
		typedef Filterable contents;
		typedef typename Filterable::scalar_type scalar_type;
		typedef typename Filterable::value_type value_type;
		typedef typename Filterable::derivative_value_type derivative_value_type;
		typedef typename Filterable::value_ptr_type value_ptr_type;
		typedef typename Filterable::derivative_filter_type derivative_filter_type;
		typedef typename Filterable::value_filter_type value_filter_type;
		typedef typename Filterable::value_return_type value_return_type;

		OneEuroFilter(scalar_type mincutoff, scalar_type beta, scalar_type dcutoff) :
			_firstTime(true),
			_mincutoff(mincutoff), _beta(beta), _dcutoff(dcutoff) {};

		OneEuroFilter() : _firstTime(true), _mincutoff(1), _beta(0.5), _dcutoff(1) {};

		void setMinCutoff(scalar_type mincutoff) {
			_mincutoff = mincutoff;
		}
		scalar_type getMinCutoff() const {
			return _mincutoff;
		}
		void setBeta(scalar_type beta) {
			_beta = beta;
		}
		scalar_type getBeta() const {
			return _beta;
		}
		void setDerivativeCutoff(scalar_type dcutoff) {
			_dcutoff = dcutoff;
		}
		scalar_type getDerivativeCutoff() const {
			return _dcutoff;
		}
		void setParams(scalar_type mincutoff, scalar_type beta, scalar_type dcutoff) {
			_mincutoff = mincutoff;
			_beta = beta;
			_dcutoff = dcutoff;
		}
		const value_return_type filter(scalar_type dt, const value_type x) {
			derivative_value_type dx;

			if (_firstTime) {
				_firstTime = false;
				Filterable::setDxIdentity(dx);

			} else {
				Filterable::computeDerivative(dx, _xfilt.hatxprev(), x, dt);
			}

			scalar_type derivative_magnitude = Filterable::computeDerivativeMagnitude(_dxfilt.filter(dx, alpha(dt, _dcutoff)));
			scalar_type cutoff = _mincutoff + _beta * derivative_magnitude;

			return _xfilt.filter(x, alpha(dt, cutoff));
		}

	private:
		static scalar_type alpha(scalar_type dt, scalar_type cutoff) {
			scalar_type tau = scalar_type(1) / (scalar_type(2) * Q_PI * cutoff);
			return scalar_type(1) / (scalar_type(1) + tau / dt);
		}

		bool _firstTime;
		scalar_type _mincutoff, _dcutoff;
		scalar_type _beta;
		value_filter_type _xfilt;
		derivative_filter_type _dxfilt;
};

typedef OneEuroFilter<> OneEuroFilterVec;

class LowPassFilterQuat {
	public:
		LowPassFilterQuat() : _firstTime(true) {
		}

		const double *filter(const q_type x, vrpn_float64 alpha) {
			if (_firstTime) {
				_firstTime = false;
				q_copy(_hatxprev, x);
			}

			q_type hatx;
			q_slerp(hatx, _hatxprev, x, alpha);
			q_copy(_hatxprev, hatx);
			return _hatxprev;
		}

		const double *hatxprev() {
			return _hatxprev;
		}

	private:
		bool _firstTime;
		q_type _hatxprev;
};

class OneEuroFilterQuat {
	public:

		typedef double value_type;
		OneEuroFilterQuat() : _firstTime(true) {};
		OneEuroFilterQuat(vrpn_float64 mincutoff, vrpn_float64 beta, vrpn_float64 dcutoff) :
			_firstTime(true),
			_mincutoff(mincutoff), _beta(beta), _dcutoff(dcutoff) {};

		void setParams(vrpn_float64 mincutoff, vrpn_float64 beta, vrpn_float64 dcutoff) {
			_mincutoff = mincutoff;
			_beta = beta;
			_dcutoff = dcutoff;
		}
		const value_type *filter(value_type dt, const q_type x) {
			q_type dx;

			if (_firstTime) {
				_firstTime = false;
				dx[0] = dx[1] = dx[2] = 0;
				dx[3] = 1;

			} else {
				q_type inverse_prev;
				q_invert(inverse_prev, _xfilt.hatxprev());
				q_mult(dx, x, inverse_prev);
			}

			q_type edx;
			q_copy(edx, _dxfilt.filter(dx, alpha(dt, _dcutoff)));
			/*
			// avoid taking acos of an invalid number due to numerical errors
			if(edx[Q_W] > 1.0) edx[Q_W] = 1.0;
			if(edx[Q_W] < -1.0) edx[Q_W] = -1.0;
			double ax,ay,az,angle;
			q_to_axis_angle(&ax, &ay, &az, &angle, edx);
			*/

			vrpn_float64 cutoff = _mincutoff + _beta * edx[Q_W];

			q_normalize(_normalizedResult, _xfilt.filter(x, alpha(dt, cutoff)));


			return _normalizedResult;
		}

	private:
		static vrpn_float64 alpha(vrpn_float64 dt, vrpn_float64 cutoff) {
			vrpn_float64 tau = (vrpn_float64)(1.0f / (2.0f * Q_PI * cutoff));
			return 1.0f / (1.0f + tau / dt);
		}

		bool _firstTime;
		value_type _mincutoff, _dcutoff;
		value_type _beta;
		value_type _normalizedResult[4];
		LowPassFilter<4, double> _xfilt, _dxfilt;
};


#endif // INCLUDED_vrpn_OneEuroFilter_h_GUID_C56A0525_3809_44B7_AA16_98711638E762

