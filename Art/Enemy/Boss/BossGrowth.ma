//Maya ASCII 2026 scene
//Name: BossGrowth.ma
//Last modified: Sat, Oct 24, 2026 02:46:09 PM
//Codeset: 932
requires maya "2026";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2026";
fileInfo "version" "2026";
fileInfo "cutIdentifier" "202504040659-cfc1e8923b";
fileInfo "osv" "Windows 11 Home v2009 (Build: 26300)";
fileInfo "UUID" "D86C23D6-4790-A825-5029-90B0D8F021B3";
createNode transform -s -n "persp";
	rename -uid "8C709E5F-4206-C206-23E8-5DB45AFF1A6B";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 28 21 28 ;
	setAttr ".r" -type "double3" -27.938352729602379 44.999999999999972 -5.172681101354183e-14 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "BDB7AA9D-4281-6600-777B-F4AE66DB38DB";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999993;
	setAttr ".coi" 44.82186966202994;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".hc" -type "string" "viewSet -p %camera";
createNode transform -s -n "top";
	rename -uid "DF8F4CC1-44A4-C1E9-AC11-EEB1F91D25B0";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 1000.1 0 ;
	setAttr ".r" -type "double3" -90 0 0 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "E0261505-4760-1329-5C79-949CC16368B5";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "top";
	setAttr ".den" -type "string" "top_depth";
	setAttr ".man" -type "string" "top_mask";
	setAttr ".hc" -type "string" "viewSet -t %camera";
	setAttr ".o" yes;
createNode transform -s -n "front";
	rename -uid "3B22DC21-4F58-9A87-A53D-94B6105AC4EF";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 1000.1 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "AF481BAB-4F55-3E0C-CC61-26A181BF1347";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "front";
	setAttr ".den" -type "string" "front_depth";
	setAttr ".man" -type "string" "front_mask";
	setAttr ".hc" -type "string" "viewSet -f %camera";
	setAttr ".o" yes;
createNode transform -s -n "side";
	rename -uid "B83EADAE-4E78-6937-5C44-42AADEBC69D7";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 1000.1 0 0 ;
	setAttr ".r" -type "double3" 0 90 0 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "0CB1100E-48BF-94C2-7F1E-2D9CACEABEF8";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
createNode transform -n "SM_BossGrowth";
	rename -uid "E6C8D474-4E81-4485-60BE-E186B90C706C";
createNode mesh -n "SM_BossGrowthShape" -p "SM_BossGrowth";
	rename -uid "AF956E2E-4EE7-635F-63C2-03857171039B";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 115 ".uvst[0].uvsp[0:114]" -type "float2" 0.046522245 0.49235398
		 0.068190932 0.49235398 0.062413428 0.59082478 0.046522245 0.59082478 0.046522245
		 0.3938832 0.07358814 0.3938832 0.055995218 0.68929559 0.046522245 0.68929559 0.024853555
		 0.49235398 0.030631062 0.59082478 0.046522245 0.29541239 0.078719109 0.29541239 0.019456325
		 0.3938832 0.047334857 0.7877664 0.046522245 0.7877664 0.037049271 0.68929559 0.046522245
		 0.1969416 0.083647393 0.1969416 0.014325398 0.29541239 0.045709632 0.7877664 0.046522245
		 0.0984708 0.088413335 0.0984708 0.0093970941 0.1969416 0.046522245 0 0.09304449 0
		 0.0046311533 0.0984708 0 0 0.45137268 0.7877664 0.37633681 0.7877664 0.37199253 0.68929559
		 0.4395588 0.68929559 0.30130094 0.7877664 0.30442628 0.68929559 0.35895962 0.59082478
		 0.41883892 0.59082478 0.27022007 0.7877664 0.2764394 0.68929559 0.29908031 0.59082478
		 0.33723813 0.49235398 0.38916856 0.49235398 0.27427751 0.59082478 0.28530768 0.49235398
		 0.30682802 0.3938832 0.3504827 0.3938832 0.2637974 0.49235398 0.26317334 0.3938832
		 0.26772931 0.29541239 0.30267882 0.29541239 0.24509096 0.3938832 0.23277982 0.29541239
		 0.21994203 0.1969416 0.24557295 0.1969416 0.21830323 0.29541239 0.1943111 0.1969416
		 0.16346613 0.0984708 0.17874511 0.0984708 0.18369442 0.1969416 0.14818715 0.0984708
		 0.098301627 0 0.099612288 0 0.14185838 0.0984708 0.096990965 0 0.096448064 0 0.65912145
		 0 0.73415732 0 0.72981298 0.0984708 0.6622467 0.0984708 0.80919319 0 0.7973792 0.0984708
		 0.71678007 0.1969416 0.65690082 0.1969416 0.62804055 0 0.63425988 0.0984708 0.77665937
		 0.1969416 0.69505858 0.29541239 0.64312816 0.29541239 0.63209796 0.1969416 0.74698901
		 0.29541239 0.66464847 0.3938832 0.62099379 0.3938832 0.62161785 0.29541239 0.70830315
		 0.3938832 0.62554979 0.49235398 0.59060031 0.49235398 0.60291141 0.3938832 0.66049933
		 0.49235398 0.57776248 0.59082478 0.55213159 0.59082478 0.57612371 0.49235398 0.60339338
		 0.59082478 0.52128661 0.68929559 0.50600761 0.68929559 0.54151487 0.59082478 0.5365656
		 0.68929559 0.4561221 0.7877664 0.45481145 0.7877664 0.49967885 0.68929559 0.45743278
		 0.7877664 0.45426854 0.7877664 0.91269773 0.8188473 0.93196785 0.89388317 0.91269773
		 0.96891904 0.86617547 1 0.81965321 0.96891904 0.80038309 0.89388317 0.81965321 0.8188473
		 0.86617547 0.7877664 0.79723585 0.7877664 0.7980485 0.78830934 0.79838502 0.78961998
		 0.7980485 0.79093063 0.79723585 0.79147363 0.79642326 0.79093063 0.79608667 0.78961998
		 0.79642326 0.78830934;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 72 ".vt[0:71]"  4.57999992 0 0 3.23854899 2.0079004765 0
		 2.8044411e-16 2.83960009 0 -3.23854899 2.0079004765 0 -4.57999992 3.477507e-16 0
		 -3.23854899 -2.0079004765 0 -8.4133236e-16 -2.83960009 0 3.23854899 -2.0079004765 0
		 3.93657279 0 4.25 2.72865987 1.80801904 4.25 -0.1875 2.55692506 4.25 -3.10365987 1.80801904 4.25
		 -4.31157255 3.1313302e-16 4.25 -3.10365987 -1.80801904 4.25 -0.1875 -2.55692506 4.25
		 2.72865987 -1.80801904 4.25 2.90488052 0 8.5 1.83439076 1.60232222 8.5 -0.75 2.26602578 8.5
		 -3.33439088 1.60232222 8.5 -4.40488052 2.7750814e-16 8.5 -3.33439088 -1.60232222 8.5
		 -0.75 -2.26602578 8.5 1.83439076 -1.60232222 8.5 1.48220158 0 12.75 0.55381745 1.38961685 12.75
		 -1.6875 1.96521497 12.75 -3.92881751 1.38961685 12.75 -4.85720158 2.4066943e-16 12.75
		 -3.92881751 -1.38961685 12.75 -1.6875 -1.96521497 12.75 0.55381745 -1.38961685 12.75
		 -0.33542871 0 17 -1.11586356 1.16816461 17 -3 1.65203416 17 -4.8841362 1.16816461 17
		 -5.66457129 2.0231584e-16 17 -4.8841362 -1.16816461 17 -3 -1.65203416 17 -1.11586356 -1.16816461 17
		 -2.55427027 0 21.25 -3.17907882 0.93522114 21.25 -4.6875 1.32260239 21.25 -6.19592094 0.93522114 21.25
		 -6.82072973 1.6197208e-16 21.25 -6.19592094 -0.93522114 21.25 -4.6875 -1.32260239 21.25
		 -3.17907882 -0.93522114 21.25 -5.18555355 0 25.5 -5.64376926 0.68586302 25.5 -6.75 0.96995676 25.5
		 -7.85623074 0.68586302 25.5 -8.31444645 1.1878544e-16 25.5 -7.85623074 -0.68586302 25.5
		 -6.75 -0.96995676 25.5 -5.64376926 -0.68586302 25.5 -8.25490952 0 29.75 -8.52805901 0.4088535 29.75
		 -9.1875 0.57820618 29.75 -9.84694099 0.4088535 29.75 -10.12009048 7.080983e-17 29.75
		 -9.84694099 -0.4088535 29.75 -9.1875 -0.57820618 29.75 -8.52805901 -0.4088535 29.75
		 -11.92000008 0 34 -11.94343185 0.035072498 34 -12 0.049600001 34 -12.056568146 0.035072498 34
		 -12.079999924 6.0742482e-18 34 -12.056568146 -0.035072498 34 -12 -0.049600001 34
		 -11.94343185 -0.035072498 34;
	setAttr -s 136 ".ed[0:135]"  0 1 0 1 9 0 9 8 1 8 0 0 1 2 0 2 10 1 10 9 1
		 2 3 0 3 11 0 11 10 1 3 4 0 4 12 0 12 11 1 4 5 0 5 13 0 13 12 1 5 6 0 6 14 1 14 13 1
		 6 7 0 7 15 0 15 14 1 7 0 0 8 15 1 9 17 0 17 16 1 16 8 0 10 18 1 18 17 1 11 19 0 19 18 1
		 12 20 0 20 19 1 13 21 0 21 20 1 14 22 1 22 21 1 15 23 0 23 22 1 16 23 1 17 25 0 25 24 1
		 24 16 0 18 26 1 26 25 1 19 27 0 27 26 1 20 28 0 28 27 1 21 29 0 29 28 1 22 30 1 30 29 1
		 23 31 0 31 30 1 24 31 1 25 33 0 33 32 1 32 24 0 26 34 1 34 33 1 27 35 0 35 34 1 28 36 0
		 36 35 1 29 37 0 37 36 1 30 38 1 38 37 1 31 39 0 39 38 1 32 39 1 33 41 0 41 40 1 40 32 0
		 34 42 1 42 41 1 35 43 0 43 42 1 36 44 0 44 43 1 37 45 0 45 44 1 38 46 1 46 45 1 39 47 0
		 47 46 1 40 47 1 41 49 0 49 48 1 48 40 0 42 50 1 50 49 1 43 51 0 51 50 1 44 52 0 52 51 1
		 45 53 0 53 52 1 46 54 1 54 53 1 47 55 0 55 54 1 48 55 1 49 57 0 57 56 1 56 48 0 50 58 1
		 58 57 1 51 59 0 59 58 1 52 60 0 60 59 1 53 61 0 61 60 1 54 62 1 62 61 1 55 63 0 63 62 1
		 56 63 1 57 65 0 65 64 0 64 56 0 58 66 1 66 65 0 59 67 0 67 66 0 60 68 0 68 67 0 61 69 0
		 69 68 0 62 70 1 70 69 0 63 71 0 71 70 0 64 71 0;
	setAttr -s 66 -ch 272 ".fc[0:65]" -type "polyFaces" 
		f 4 0 1 2 3
		mu 0 4 23 24 21 20
		f 4 4 5 6 -2
		mu 0 4 27 28 29 30
		f 4 7 8 9 -6
		mu 0 4 28 31 32 29
		f 4 10 11 12 -9
		mu 0 4 31 35 36 32
		f 4 13 14 15 -12
		mu 0 4 71 63 66 72
		f 4 16 17 18 -15
		mu 0 4 63 64 65 66
		f 4 19 20 21 -18
		mu 0 4 64 67 68 65
		f 4 22 -4 23 -21
		mu 0 4 26 23 20 25
		f 4 -3 24 25 26
		mu 0 4 20 21 17 16
		f 4 -7 27 28 -25
		mu 0 4 30 29 33 34
		f 4 -10 29 30 -28
		mu 0 4 29 32 37 33
		f 4 -13 31 32 -30
		mu 0 4 32 36 40 37
		f 4 -16 33 34 -32
		mu 0 4 72 66 70 76
		f 4 -19 35 36 -34
		mu 0 4 66 65 69 70
		f 4 -22 37 38 -36
		mu 0 4 65 68 73 69
		f 4 -24 -27 39 -38
		mu 0 4 25 20 16 22
		f 4 -26 40 41 42
		mu 0 4 16 17 11 10
		f 4 -29 43 44 -41
		mu 0 4 34 33 38 39
		f 4 -31 45 46 -44
		mu 0 4 33 37 41 38
		f 4 -33 47 48 -46
		mu 0 4 37 40 44 41
		f 4 -35 49 50 -48
		mu 0 4 76 70 75 80
		f 4 -37 51 52 -50
		mu 0 4 70 69 74 75
		f 4 -39 53 54 -52
		mu 0 4 69 73 77 74
		f 4 -40 -43 55 -54
		mu 0 4 22 16 10 18
		f 4 -42 56 57 58
		mu 0 4 10 11 5 4
		f 4 -45 59 60 -57
		mu 0 4 39 38 42 43
		f 4 -47 61 62 -60
		mu 0 4 38 41 45 42
		f 4 -49 63 64 -62
		mu 0 4 41 44 48 45
		f 4 -51 65 66 -64
		mu 0 4 80 75 79 84
		f 4 -53 67 68 -66
		mu 0 4 75 74 78 79
		f 4 -55 69 70 -68
		mu 0 4 74 77 81 78
		f 4 -56 -59 71 -70
		mu 0 4 18 10 4 12
		f 4 -58 72 73 74
		mu 0 4 4 5 1 0
		f 4 -61 75 76 -73
		mu 0 4 43 42 46 47
		f 4 -63 77 78 -76
		mu 0 4 42 45 49 46
		f 4 -65 79 80 -78
		mu 0 4 45 48 52 49
		f 4 -67 81 82 -80
		mu 0 4 84 79 83 88
		f 4 -69 83 84 -82
		mu 0 4 79 78 82 83
		f 4 -71 85 86 -84
		mu 0 4 78 81 85 82
		f 4 -72 -75 87 -86
		mu 0 4 12 4 0 8
		f 4 -74 88 89 90
		mu 0 4 0 1 2 3
		f 4 -77 91 92 -89
		mu 0 4 47 46 50 51
		f 4 -79 93 94 -92
		mu 0 4 46 49 53 50
		f 4 -81 95 96 -94
		mu 0 4 49 52 56 53
		f 4 -83 97 98 -96
		mu 0 4 88 83 87 92
		f 4 -85 99 100 -98
		mu 0 4 83 82 86 87
		f 4 -87 101 102 -100
		mu 0 4 82 85 89 86
		f 4 -88 -91 103 -102
		mu 0 4 8 0 3 9
		f 4 -90 104 105 106
		mu 0 4 3 2 6 7
		f 4 -93 107 108 -105
		mu 0 4 51 50 54 55
		f 4 -95 109 110 -108
		mu 0 4 50 53 57 54
		f 4 -97 111 112 -110
		mu 0 4 53 56 60 57
		f 4 -99 113 114 -112
		mu 0 4 92 87 91 96
		f 4 -101 115 116 -114
		mu 0 4 87 86 90 91
		f 4 -103 117 118 -116
		mu 0 4 86 89 93 90
		f 4 -104 -107 119 -118
		mu 0 4 9 3 7 15
		f 4 -106 120 121 122
		mu 0 4 7 6 13 14
		f 4 -109 123 124 -121
		mu 0 4 55 54 58 59
		f 4 -111 125 126 -124
		mu 0 4 54 57 61 58
		f 4 -113 127 128 -126
		mu 0 4 57 60 62 61
		f 4 -115 129 130 -128
		mu 0 4 96 91 95 98
		f 4 -117 131 132 -130
		mu 0 4 91 90 94 95
		f 4 -119 133 134 -132
		mu 0 4 90 93 97 94
		f 4 -120 -123 135 -134
		mu 0 4 15 7 14 19
		f 8 -20 -17 -14 -11 -8 -5 -1 -23
		mu 0 8 99 100 101 102 103 104 105 106
		f 8 -122 -125 -127 -129 -131 -133 -135 -136
		mu 0 8 107 108 109 110 111 112 113 114;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ndt" 0;
createNode lightLinker -s -n "lightLinker1";
	rename -uid "A2958696-4741-79E9-3A4B-B991AF25D282";
	setAttr -s 2 ".lnk";
	setAttr -s 2 ".slnk";
createNode displayLayerManager -n "layerManager";
	rename -uid "7FCC1EBC-4B84-6301-0BBB-6DB1F8EA75F7";
createNode displayLayer -n "defaultLayer";
	rename -uid "D018F2FD-4FE9-6A8A-F122-DEB776D0CAB7";
	setAttr ".ufem" -type "stringArray" 0  ;
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "3E6CB5E6-4BF6-B161-5C6A-DB86EE99AC9F";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "887D5C13-4759-8A4B-A97D-C295CA60FDB1";
	setAttr ".g" yes;
createNode script -n "uiConfigurationScriptNode";
	rename -uid "5130780E-4A4B-2B23-EF5B-31A99F4211DF";
	setAttr ".b" -type "string" "// Maya Mel UI Configuration File.\n// No UI generated in batch mode.\n";
	setAttr ".st" 3;
createNode script -n "sceneConfigurationScriptNode";
	rename -uid "879852B2-4BE1-7253-8462-0EA675FD1A29";
	setAttr ".b" -type "string" "playbackOptions -min 1 -max 120 -ast 1 -aet 200 ";
	setAttr ".st" 6;
select -ne :time1;
	setAttr ".o" 1;
	setAttr ".unw" 1;
select -ne :hardwareRenderingGlobals;
	setAttr ".otfna" -type "stringArray" 22 "NURBS Curves" "NURBS Surfaces" "Polygons" "Subdiv Surface" "Particles" "Particle Instance" "Fluids" "Strokes" "Image Planes" "UI" "Lights" "Cameras" "Locators" "Joints" "IK Handles" "Deformers" "Motion Trails" "Components" "Hair Systems" "Follicles" "Misc. UI" "Ornaments"  ;
	setAttr ".otfva" -type "Int32Array" 22 0 1 1 1 1 1
		 1 1 1 0 0 0 0 0 0 0 0 0
		 0 0 0 0 ;
	setAttr ".fprt" yes;
	setAttr ".rtfm" 1;
select -ne :renderPartition;
	setAttr -s 2 ".st";
select -ne :renderGlobalsList1;
select -ne :defaultShaderList1;
	setAttr -s 6 ".s";
select -ne :postProcessList1;
	setAttr -s 2 ".p";
select -ne :defaultRenderingList1;
select -ne :standardSurface1;
	setAttr ".bc" -type "float3" 0.40000001 0.40000001 0.40000001 ;
	setAttr ".sr" 0.5;
select -ne :openPBR_shader1;
	setAttr ".bc" -type "float3" 0.40000001 0.40000001 0.40000001 ;
	setAttr ".sr" 0.5;
select -ne :initialShadingGroup;
	setAttr ".ro" yes;
select -ne :initialParticleSE;
	setAttr ".ro" yes;
select -ne :defaultRenderGlobals;
	addAttr -ci true -h true -sn "dss" -ln "defaultSurfaceShader" -dt "string";
	setAttr ".dss" -type "string" "openPBR_shader1";
select -ne :defaultResolution;
	setAttr ".pa" 1;
select -ne :defaultColorMgtGlobals;
	setAttr ".cfe" yes;
	setAttr ".cfp" -type "string" "<MAYA_RESOURCES>/OCIO-configs/Maya2022-default/config.ocio";
	setAttr ".vtn" -type "string" "ACES 1.0 SDR-video (sRGB)";
	setAttr ".vn" -type "string" "ACES 1.0 SDR-video";
	setAttr ".dn" -type "string" "sRGB";
	setAttr ".wsn" -type "string" "ACEScg";
	setAttr ".otn" -type "string" "ACES 1.0 SDR-video (sRGB)";
	setAttr ".potn" -type "string" "ACES 1.0 SDR-video (sRGB)";
select -ne :hardwareRenderGlobals;
	setAttr ".ctrs" 256;
	setAttr ".btrs" 512;
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
connectAttr "layerManager.dli[0]" "defaultLayer.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer.rlid";
connectAttr "defaultRenderLayer.msg" ":defaultRenderingList1.r" -na;
// End of BossGrowth.ma
