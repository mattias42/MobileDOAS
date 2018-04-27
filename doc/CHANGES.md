MobileDOAS Release Notes
-----------------------------------------------------
Version 6.0 (April 2018)

Critical changes
* Support for 64-bit OS

New features
* Add option to remove points with intensity higher than a certain threshold (#17)
* Use another color (cyan) for fit resolve in Fit Window (#19)
* Auto-scale real-time route (#20)
* Automatically change flux calculations when unit is changed (#23)
* Display absolute values for final plume width and flux calculations (#24, #25)
* Add support for re-evaluation of measurements taken in adaptive mode (#26)
* Add Altitude to .STD files (#32)

Bug fixes
* Fix issue with list of references not updating in Configuration Dialog->Evalute (#11)
* Validate lat/lon values after focus is removed from field in Post Flux Calculation dialog (#14)
* Fix issue with intensity slide bar on main page not always showing up (#21)
* Fix date problem with measurements spanning midnight UTC (#27)
* Fix Altitude showing up as 0.0 in evaluation logs (#29)
* Fix "Could not communicate with the GPS" error popping up behind main UI (#31)
* Fix multiples of same tab being generated in Post Flux Calculation dialog when 'File->ReEvaluate This Log File' is selected (#34)
* Add message for user when Post-Wind calculations are not supported (#39)
* Fix compatibility with high DPI display (#47)
* Fix issue with reading GPS data off of COM ports above 9 (#53)

Removal of unused features
* Remove option 3 from 'Sky' tab in ReEvaluation dialog (#9)
* Remove 'Dark' tab from ReEvaluation dialog (#10)
* Disable WindSpeed Measurement and PlumeHeight Measurement menu items (until dual-beam support can be implemented)
* Remove Change Exposure Time menu option (#43)