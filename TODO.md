# RTKLIB ToDo List

## Command line options for start and end time

The command line options for the start and end time and date behave differently for `rtkpost_qt` and `rnx2rtkp`. For `rtkpost_qt`, two hyphens and quotation marks are required.

```
rtkpost_qt --ts "2023/01/01 01:00:00" --te "2023/01/01 02:00:00"
```

For `rnx2rtkp`, the command line option uses a single hyphen and no quotation marks

```
rnx2rtkp -ts 2023/01/01 01:00:00 -te 2023/01/01 02:00:00
```

## Inconsistencies of solution *.stat file content

There are inconsistencies between the pdf documentation and the sourcecode comments and the implementaiton of the output ot the `*.stat file.

The output of the solution happens in `rtkpos.c:rtkoutstat()`. In case the positioning mode `PMODE_PPP_KINEMA` or higher is selected, the function `ppp.c:pppoutstat()` is used for the output instead.

## Meaning of PPP modes and associated process noise setting for position states

1. `PMODE_PPP_FIXED`: the position is reset at each epoch to the pre-defined user position in the configuration file `ant2-pos1` (#TBC!#) with a small variance of 1.0e8 m^2^.

1. `PMODE_PPP_STATIC`: a process noise with a variance defined in `stats-prnpos` in the configuration file is applied to the position state. The default value is 0.0  m^2^/s, it cannot be changed in the GUI!

1. `PMODE_PPP_KINEMA`: if receiver dynamics are not activated, a hard-coded process noise with a variance of 60 m^2^/s is applied to the variance of the position states. This value is also used for the intial variance of the position state. If receiver dynamics are used, process noise with a variances defined in `stats-prnaccelh` and `stats-prnaccelv` in the configuraiton file is applied to the acceleration state.

## Selection of observation types

Review the selection of observations in `rinex.c:decode_obsdata` with respect to code priorities!

## Binary files in repository

The folders 'dll' and 'lib' contain binary files. 

## Processing of PCO/PCV from ANTEX

Review the processing of PCOs and PCVs in `rtkcmn.c:readantex()` and make sure, that the correct values are processed.

The data structure defines frequency numbers 1,2 and 5 for the three GPS frequencies L1,L2 and L5. For Galileo 1 and 5 are used for E1 and E5a, 2 is used for E5b, E6 is ignored. For the other GNSSs, frequencies which match the numbers 1,2 and 5 are stored, others are ignored. This may cause issue in particular for BeiDou.

  | Number | 1 | 2 | 3 |  
  | GNSS   |  |  |  |  
  |:-|:-|:-|:-|  
  | GPS          | L1 | L2 | L5 |  
  | Galileo      | E1 | E5b | E5a |  
  | GLONASS      | G1 | G2 | - |  
  | BeiDou       | ?? | ?? | ?? |  
  | QZSS         | L1 | L2 | L5 |  

## Satellite and receiver PCO/PCV handling

The satellite PCO and PCV corrections are applied in different places in the source code.

Satellite PCO corrections are directly applied to the computed satellite position as a 3-dimensional vector in ECEF for positions from SP3 files. The default implementation in `preceph.c:satantoff()` forms the ionosphere-free combination of two preselected signals and computes the combined PCO correction. (NOTE: this can be turned off by setting a flag `OPT`.)

Satellite PCV corrections are computed through two functions: the function `ppp.c:satantpcv()` computes the nadir angle and then calls `rtkcmn.c:antmodel_s()` to compute teh interpolated PCV correction as a scalar value for each frequency. 

Receiver PCO and PCV corrections are jointly computed in the function `rtkcmn.c:antmodel()`. The PCO is projected onto the LOS vector and the PCV is interpolated depending on the satellite elevation angle. Applying the PCV correction can optionally be turned off. This function also applies the eccentricity correction.

## Satellite position and APC handling

The precise position returned by `satpos()` refers to the iono-free APC instead of COM for SP3. This comes with the advantage that the satellite antenna PCO is already applied at this stage. Only the correction for the PCV must then be applied for each frequency, which only depends on the Nadir angle. However, this will cause problems for consistent modeling of PCOs for un-combined multi-frequency PPP.

The satellite position for PPP is computed in `ppp.c:pppos()`. From there, the following functions are called:

1. `ephemeris.c:satposs()`

Computes position, clock offset, etc. for each satellite in the observation vector. Calls `ephemeris.c:satpos()` for each satellite.

2. `ephemeris.c:satpos()`

Computes position, clock offset, etc. for single satellite. Uses a switch `opt` to set teh computed positio to either CoM (`opt=0`) or APC (`opt=1`) depending on ephemeris type. Calls `preceph.c:peph2pos()` with `opt=1` when the ephemeris type `EPHOPT_PREC` is selected.

3. `preceph.c:peph2pos()`

Computes the satellite position based on SP3. Here, the reference can be set either to CoM or APC using the `opt` switch. Calls the function `preceph.c:satantoff()` to compute the iono-free PCO.

## Ephemeris Option QZSS LEX

A ephemeris option `QZSS LEX` is defined in the GUIs, but does not seem to be included in the RTKLIB option set.

## Carrier-phase biases in Bias-SINEX

Carrier-phase biases are currently not supported. A data-structure for such biases is missing.

## Code bias handling for DSB and OSB

Not all OSBs in Bias-SINEX files seem to be supported. The OSBs are internally converted into DCBs by subtracting the values from a reference bias (with index 0) in `preceph.c::readbiaf()`. In the case of CODE MGEX files, the biases for the code reference signals `C1W` and `C2W` are set to zero. As a result, the stored biases are the negative values of the other biases. 

TODO: check which values are actually stored in the data structures.

The code biases are applied in `ppp.c:corr_meas()`, where they are ADDED to the observations. Due to the above sign inversion, this seems to result in the correct sign. 

TODO: check what happens in case of a third frequency.

Ideally, the OSBs should simply be used directly without any further modification. A flag could be used for indicating if absolute or relative biases have been read. 

## Bias file input in rtkpost_qt

After starting the processing `postpos.c:execses()` first checks if any of the files defined in the main GUI is a bias file. If no biases are found, a DCB file defined under `Options->Files` is read.

## Missing slot in rtkplot_qt

The program `rtkplot_qt` misses the slot `MenuShapeFileClick()`

```
QObject::connect: No such slot Plot::MenuShapeFileClick() in plotmain.cpp:308
QObject::connect:  (sender name:   'MenuShapeFile')
QObject::connect:  (receiver name: 'Plot')
```

## Pre-fit measurement residuals

The pre-fit measurement residuals in `ppp_res()` are not corrected for by common offset due to receiver clock errors. This requires a unreasonably large editing threshold to keep the observations in.

## Variance of ionosphere-free combination

A fixed scaling factor of 3 is used for the observation standard deviation of the IF combination.

## Definition of compilation options

The following options (c.f. `rtklib.h`) must be defined at compilation time:

```
ENAGLO 
ENAQZS 
ENAGAL 
ENACMP 
ENAIRN 
NFREQ
NEXOBS
IERS_MODEL
```
The file `RTKLIB.pri` in the root directory controls this universally for all QT applications. For the CUI applications, this must be individually activated in each makefile!
