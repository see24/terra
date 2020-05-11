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

#include "spatVector.h"

#ifdef useGDAL

#include "file_utils.h"
#include "ogrsf_frmts.h"


bool SpatVector::write(std::string filename, std::string lyrname, std::string driver, bool overwrite) {

    GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName( driver.c_str() );
    if( poDriver == NULL )  {
        setError( driver + " driver not available");
        return false;
    }

    GDALDataset *poDS;
    poDS = poDriver->Create(filename.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL ) {
        setError("Creation of output file failed" );
        return false;
    }

	OGRwkbGeometryType wkb;
	SpatGeomType geomtype = lyr.geoms[0].gtype;
	if (geomtype == points) {
		wkb = wkbPoint;
	} else if (geomtype == lines) {
		wkb = wkbMultiLineString;
	} else if (geomtype == polygons) {
		wkb = wkbMultiPolygon;
	} else {
        setError("this geometry type is not supported");
        return false;			
	}


	OGRSpatialReference *srs = NULL;
	std::string s = lyr.srs.wkt;
	if (s != "") {
		srs = new OGRSpatialReference;
		OGRErr err = srs->SetFromUserInput(s.c_str()); 
		if (err != OGRERR_NONE) {
			setError("crs error");
			delete srs;
			return false;
		}
	}
	
    OGRLayer *poLayer;
    poLayer = poDS->CreateLayer(lyrname.c_str(), srs, wkb, NULL );
    if( poLayer == NULL ) {
        setError( "Layer creation failed" );
        return false;
    }
	srs->Release();

	std::vector<std::string> nms = get_names();
	std::vector<std::string> tps = lyr.df.get_datatypes();
	OGRFieldType otype;
	int nfields = nms.size();
	size_t ngeoms = size();
	
	for (int i=0; i<nfields; i++) {
		if (tps[i] == "double") {
			otype = OFTReal;
		} else if (tps[i] == "long") {
			otype = OFTInteger64;
		} else {
			otype = OFTString;
		}

		OGRFieldDefn oField(nms[i].c_str(), otype);
		if (otype == OFTString) {
			oField.SetWidth(32);
		}
		if( poLayer->CreateField( &oField ) != OGRERR_NONE ) {
			setError( "Creating Name field failed" );
			return false;
		}
	}
	
	unsigned r = 0;
	OGRPoint pt;

	for (size_t i=0; i<ngeoms; i++) {
		
		OGRFeature *poFeature;
        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
		for (int j=0; j<nfields; j++) {
			if (tps[j] == "double") {
				poFeature->SetField(j, lyr.df.getDvalue(r, j));
			} else if (tps[j] == "long") {
				poFeature->SetField(j, (GIntBig)lyr.df.getIvalue(r, j));
			} else {
				poFeature->SetField(j, lyr.df.getSvalue(r, j).c_str());
			}
		}
		r++;
	
// points -- also need to do mutlipoints
		if (wkb == wkbPoint) {
			SpatGeom g = getGeom(i);
			pt.setX( g.parts[0].x[0] );
			pt.setY( g.parts[0].y[0] );
			poFeature->SetGeometry( &pt );
			
// lines			
		} else if (wkb == wkbMultiLineString) {
			SpatGeom g = getGeom(i);
			OGRMultiLineString poGeom;
			for (size_t j=0; j<g.size(); j++) {
				OGRLineString poLine;
				SpatPart p = g.getPart(j);
				for (size_t k=0; k<p.size(); k++) {
					pt.setX(p.x[k]);
					pt.setY(p.y[k]);
					poLine.setPoint(k, &pt);
				}
				if (poGeom.addGeometry(&poLine) != OGRERR_NONE ) {
					setError("cannot add line");
					return false;
				}	
			}
			if (poFeature->SetGeometry( &poGeom ) != OGRERR_NONE) {
				setError("cannot set geometry");
				return false;
			}
			
// polygons			
		} else if (wkb == wkbMultiPolygon) {
			SpatGeom g = getGeom(i);
			OGRPolygon poGeom;
			for (size_t j=0; j<g.size(); j++) {
				OGRLinearRing poRing;
				SpatPart p = g.getPart(j);
				for (size_t k=0; k<p.size(); k++) {
					pt.setX(p.x[k]);
					pt.setY(p.y[k]);
					poRing.setPoint(k, &pt);
				}
				if (poGeom.addRing(&poRing) != OGRERR_NONE ) {
					setError("cannot add ring");
					return false;
				}
				
				if (p.hasHoles()) {
					for (size_t h=0; h < p.nHoles(); h++) {
						SpatHole hole = p.getHole(h);
						OGRLinearRing poHole;
						for (size_t k=0; k<hole.size(); k++) {
							pt.setX(hole.x[k]);
							pt.setY(hole.y[k]);
							poHole.setPoint(k, &pt);
						}						
						if (poGeom.addRing(&poHole) != OGRERR_NONE ) {
							setError("cannot add hole");
							return false;
						}
					}
				}
				//closeRings
			}
			if (poFeature->SetGeometry( &poGeom ) != OGRERR_NONE) {
				setError("cannot set geometry");
				return false;
			}
		} else {
			setError("Only points and lines are currently supported");
			return false;
		}
		
		if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE ) {
			setError("Failed to create feature");
			return false;
        }

        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
	return true;
}


/*
bool SpatVector::write(std::string filename, std::string lyrname, std::string driver, bool overwrite) {
    GDALDataset* poDS=NULL;
	bool success = write_ogr(poDS, filename, lyrname, driver, overwrite);
    GDALClose( poDS );
	return success;
}


bool SpatVector::write_GDAL_ds(GDALDataset* poDS) {
	return write_ogr(poDS, "", "lyr", "Memory", true);
}

*/

#endif