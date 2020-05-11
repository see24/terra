// Copyright (c) 2018-2020  Robert J. Hijmans
//
// This file is part of the "spat" library.
//
// spat is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// spat is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with spat. If not, see <http://www.gnu.org/licenses/>.


#include "spatRaster.h"


bool SpatRaster::writeValuesMem(std::vector<double> &vals, unsigned startrow, unsigned nrows, unsigned startcol, unsigned ncols) {
	size_t sz = source[0].values.size();
	size_t vsz = vals.size();
	size_t totsz = size();
	if (vsz == totsz) {
		source[0].values = vals;
		return true;
	} 


	if (sz == 0) {
		source[0].values = std::vector<double>(totsz, NAN);
	}
	//size_t offset = startrow * ncol();
	size_t nc = ncell();
	size_t chunk = nrows * ncols;

	if (startcol==0 && ncols==ncol()) {
		for (size_t i=0; i<nlyr(); i++) {
			size_t off1 = i * chunk; 
			size_t off2 = startrow * ncols + i * nc; 
			//for (size_t j=0; j<chunk; j++) {
			//	source[0].values[off2+j] <- vals[off1+j];
			//}
			std::copy( vals.begin()+off1, vals.begin()+off1+chunk, source[0].values.begin()+off2 );
		}
	} else { // block writing
		for (size_t i=0; i<nlyr(); i++) {
			unsigned off = i*chunk;
			for (size_t r=0; r<nrows; r++) {
				size_t start = r * ncols + off;
				std::vector<double> v(vals.begin()+start, vals.begin()+start+ncols);
				start = (startrow+r)*ncol() + i*nc + startcol;
				std::copy(v.begin(), v.end(), source[0].values.begin()+start);
			}
		}
	}
	return true;
}


/*

bool SpatRaster::writeValuesMem(std::vector<double> &vals, unsigned startrow, unsigned nrows, unsigned startcol, unsigned ncols) {
	size_t sz = source[0].values.size();
	size_t start = startrow * ncol() * nlyr();
	if (((startcol==0) & (ncols==ncol())) & ((sz == start) | (sz ==0))) {
		if (sz == 0) { // first or all
			source[0].values = vals;
		} else if (sz == start) { // in chunks
			source[0].values.insert(source[0].values.end(), vals.begin(), vals.end());
		}
//		} else { // async
//			if (start+vals.size() > sz) {
//				source[0].values.resize(start+vals.size(), NAN);
//			}
//			for (size_t i=0; i<vals.size(); i++) {
//				source[0].values[start+i] = vals[i];
//			}
//		}

	} else { // block writing
		if (sz == 0) {
			source[0].values.resize(size(), NAN);
		}
		unsigned nc1 = nrows*ncols;
		unsigned nc2 = ncell();
		for (size_t i=0; i<nlyr(); i++) {
			unsigned off = i*nc1;
			for (size_t r=0; r<nrows; r++) {
				size_t start = r * ncols + off;
				std::vector<double> v(vals.begin()+start, vals.begin()+start+ncols);
				start = (startrow+r)*ncol() + i*nc2 + startcol;
				std::copy(v.begin(), v.end(), source[0].values.begin()+start);
			}
		}
	}
	return true;
}


*/