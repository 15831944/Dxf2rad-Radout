// **********
// radout.dcl  for Radout 4.0.1
// **********

dcl_settings : default_dcl_settings { audit_level = 0; }

radout : dialog
{
	label = "radout - radiance Export Facility";
	: column {
		: row {
            : boxed_column {
                label  = "Geometry to Export:";
                key    = "fileList";
                : toggle {
                    label = "3DFACEs";
                    key   = "3DFACE";
                    value = "1";
                }
                : toggle {
                    label = "Extruded TRACEs";
                    key   = "TRACE";
                    value = "1";
                }
                : toggle {
                    label = "Extruded SOLIDs";
                    key   = "SOLID";
                    value = "1";
                }
                : toggle {
                    label = "CIRCLEs";
                    key   = "CIRCLE";
                    value = "1";
                }
                : toggle {
                    label = "Extruded ARCs";
                    key   = "ARC";
                    value = "1";
                }
                : toggle {
                    label = "Extruded LINEs";
                    key   = "LINE";
                    value = "1";
                }
                : toggle {
                    label = "Extruded 2D-PLINEs";
                    key   = "PLINE";
                    value = "1";
                }
                : toggle {
                    label = "Wide 2D-PLINES";
                    key   = "WPLINE";
                    value = "1";
                }
                : toggle {
                    label = "3D-MESHes";
                    key   = "PMESH";
                    value = "1";
                }
                : toggle {
                    label = "POLYFACEs";
                    key   = "PFACE";
                    value = "1";
                }
                : toggle {
                    label = "Closed 2D-PLINES";
                    key   = "POLYGON";
                    value = "0";
                }
                : toggle {
                    label = "POINTs as Spheres";
                    key   = "POINT";
                    value = "0";
                }
                : toggle {
                    label = "ACIS 3d Solids";
                    key   = "3DSOLID";
                    value = "1";
					disabled = true;
                }
                : toggle {
                    label = "ACIS Bodies";
                    key   = "BODY";
                    value = "1";
					disabled = true;
                }
                : toggle {
                    label = "ACIS Regions";
                    key   = "REGION";
                    value = "1";
					disabled = true;
                }
            }
            : column {
                : boxed_column {
                    label = "Write Filetypes:";
                    : toggle {
                        label = "Geometry Files";
                        key   = "geometry";
                        value = "1";
                    }
                    : toggle {
                        label = "Materials File";
                        key   = "Materials";
                        value = "1";
                    }
                    : toggle {
                        label = "Radiance Input File (rif)";
                        key   = "rif";
                        value = "0";
                    }
                    : toggle {
                        label = "Exterior Daylight File:";
                        key   = "sunlight";
                        value = "0";
                    }
                    : row {
                        key = "sunvals";
                        : column {
                            : edit_box {
                                label = " Month(0-12):";
                                key   = "Month";
                                value = "3";
								edit_width = 10;
                            }
                            : edit_box {
                                label = " Day(0-31):  ";
                                key   = "Day";
                                value = "22";
								edit_width = 10;
                            }
                            : edit_box {
                                label = " Hour(0-24): ";
                                key   = "Hour";
                                value = "14.5";
								edit_width = 10;
                            }
                        }
                        :column {
                            :edit_box {
                                label = "Long.:";
                                key   = "Long";
                                value = "71.1";
								edit_width = 10;
                            }
                            :edit_box {
                                label = "Lat.: ";
                                key   = "Lat";
                                value = "42.3";
								edit_width = 10;
                            }
                            :edit_box {
                                label = "T.Z.: ";
                                key   = "TZ";
                                value = "5";
								edit_width = 10;
                            }
                        }
                    }
                }
                : boxed_column {
                    label = "Sampling Modes:";
                    key   = "modes";
                    : row {
                        :text {
                            label = "Assign Materials by:";
                        }
                        : radio_row {
                            key   = "sample";
                            value = "Color";
                            :radio_button {
                                label = "Color ";
                                key   = "Color";
                            }
                            :radio_button {
                                label = "Layer ";
                                key   = "Layer";
                            }
                        }
                    }
//                    : row {
//                        key  = "smooth";
//						is_enabled = false;
//                        : toggle {
//							is_enabled = false;
//                            label = "Smoothing";
//                            key   = "smoothing";
//                            value = "0";
//                            edit_width = 10;
//                        }
//                        : edit_box {
//							is_enabled = false;
//                            label = "Degrees Tolerance:";
//                            key   = "tolerance";
//                            value = "90";
//                            edit_width = 10;
//                        }
//                    }
					: edit_box {
                        label = "Distance Tolerance for Arc Approx.:";
                        key   = "distTolerance";
                        value = ".1";
						edit_width = 10;
                    }
                    : edit_box {
                        label = "Angle Tolerance for Arc Approx.:";
                        key   = "angleTolerance";
                        value = "15.0";
						edit_width = 10;
                    }
					: edit_box {
						label = "Ouput Scaling Factor:";
						key   = "scale";
						value = "1.0";
						edit_width = 10;
					}
				}
                : boxed_column {
                    label = "File Names:";
					: edit_box {
						label = "Basename for Output:";
						key   = "basename";
						edit_width = 30;
					}
                }
            }
        }
        ok_cancel;
        errtile;
    }
}

/*** radout.dcl ***/
