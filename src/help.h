const char *help_help =
"Help topics\n"
"===========\n"
"Type 'off_util -H topic_name' for help on each topic\n"
"\n"
"col_map : colour map specification and resource colour maps\n"
"models:   builtin models\n"
"   uniform: uniform polyhedra\n"
"   johnson: johnson polyhedra\n"
"   uc:      uniform compounds\n";


const char *help_models =
"Models\n"
"======\n"
"Antiprism includes a number of builtin models. These may be used\n"
"by any program that expects an OFF file, by passing the model name\n"
"instead of a file name e.g. antiview u1\n"
"\n"
"See the help topic for each model type for more details\n"
"   uniform: uniform polyhedra\n"
"   johnson: johnson polyhedra\n"
"   polygon: polygon-based polyhedra such as prisms, antiprisms,\n"
"            pyramids, dipyramids, cupolas, orthobicupolas, gyrobicupolas\n"
"            and snub-antiprisms\n"
"   uc:      uniform compounds\n";


const char *help_uniform =
"Uniform Polyhedra\n"
"=================\n"
"Uniform polyhdra can be specified by\n"
"\n"
"o   A U number e.g. u8\n"
"\n"
"o   u_ followed by a Wythoff Symbol e.g. \"u_2 4|3\"\n"
"\n"
"o   u_ followed by a name (see the list below) Use '_' instead of\n"
"    a space to avoid having to quote the model name. The beginning\n"
"    of a name can be given and the first match from the list of\n"
"    uniform polyhedra below is returned.\n"
"\n"
"    When giving a name the following abbreviations can be used\n"
"       tr:     truncated\n"
"       sm:     small\n"
"       gr:     great\n"
"       st:     stellated\n"
"       sn:     snub\n"
"       tet:    tetrahedron\n"
"       ico:    icosahedron\n"
"       icosa:  icosahedron\n"
"       dod:    dodecahedron\n"
"       oct:    octahedron\n"
"       cubo:   cuboctahedron\n"
"       icosid: icosidodecahedron\n"
"\n"
"    e.g. u_truncated_octahedron, u_tr_octahedron, u_tr_oct, u_tr_o\n"
"\n"
"o   Common polyhedra are given by name only\n"
"\n"
"     tet                    u1\n"
"     tetrahedron            u1\n"
"     tr_tetrahedron         u2\n"
"     tr_tet                 u2\n"
"     octahedron             u5\n"
"     oct                    u5\n"
"     cube                   u6\n"
"     cuboctahedron          u7\n"
"     cubo                   u7\n"
"     tr_octahedron          u8\n"
"     tr_oct                 u8\n"
"     tr_cube                u9\n"
"     rhombicuboctahedron    u10\n"
"     rhombicubo             u10\n"
"     tr_cuboctahedron       u11\n"
"     tr_cubo                u11\n"
"     snub_cube              u12\n"
"     icosahedron            u22\n"
"     ico                    u22\n"
"     icosa                  u22\n"
"     dodecahedron           u23\n"
"     dod                    u23\n"
"     icosidodecahedron      u24\n"
"     icosid                 u24\n"
"     tr_icosahedron         u25\n"
"     tr_ico                 u25\n"
"     tr_icosa               u25\n"
"     tr_dodecahedron        u26\n"
"     tr_dod                 u26\n"
"     rhombicosidodecahedron u27\n"
"     rhombicosid            u27\n"
"     tr_icosidodecahedron   u28\n"
"     tr_icosid              u28\n"
"     snub_dodecahedron      u29\n"
"     snub_dod               u29\n"
"\n"
"o    Uniform prism are named 'pri' followed by the polygon\n"
"     fraction e.g pri5, pri5/2\n"
"\n"
"o    Uniform antiprism are named 'ant' followed by the polygon\n"
"     fraction e.g ant5, ant5/2, ant5/3\n"
"\n"
"Uniform List:\n"
"\n"
"U No.  Wythoff Sym    Name\n"
"-----  -----------    ------------------------\n"
"U1           3|2 3    tetrahedron\n"
"U2           2 3|3    truncated tetrahedron\n"
"U3         3/2 3|3    octahemioctahedron\n"
"U4         3/2 3|2    tetrahemihexahedron\n"
"U5           4|2 3    octahedron\n"
"U6           3|2 4    cube\n"
"U7           2|3 4    cuboctahedron\n"
"U8           2 4|3    truncated octahedron\n"
"U9           2 3|4    truncated cube\n"
"U10          3 4|2    rhombicuboctahedron\n"
"U11         2 3 4|    truncated cuboctahedron\n"
"U12         |2 3 4    snub cube\n"
"U13        3/2 4|4    small cubicuboctahedron\n"
"U14        3 4|4/3    great cubicuboctahedron\n"
"U15        4/3 4|3    cubohemioctahedron\n"
"U16       4/3 3 4|    cubitruncated cuboctahedron\n"
"U17        3/2 4|2    great rhombicuboctahedron\n"
"U18       3/2 2 4|    small rhombihexahedron\n"
"U19        2 3|4/3    stellated truncated hexahedron\n"
"U20       4/3 2 3|    great truncated cuboctahedron\n"
"U21     4/3 3/2 2|    great rhombihexahedron\n"
"U22          5|2 3    icosahedron\n"
"U23          3|2 5    dodecahedron\n"
"U24          2|3 5    icosidodecahedron\n"
"U25          2 5|3    truncated icosahedron\n"
"U26          2 3|5    truncated dodecahedron\n"
"U27          3 5|2    rhombicosidodecahedron\n"
"U28         2 3 5|    truncated icosidodecahedron\n"
"U29         |2 3 5    snub dodecahedron\n"
"U30        3|5/2 3    small ditrigonal icosidodecahedron\n"
"U31        5/2 3|3    small icosicosidodecahedron\n"
"U32       |5/2 3 3    small snub icosicosidodecahedron\n"
"U33        3/2 5|5    small dodecicosidodecahedron\n"
"U34        5|2 5/2    small stellated dodecahedron\n"
"U35        5/2|2 5    great dodecahedron\n"
"U36        2|5/2 5    great dodecadodecahedron\n"
"U37        2 5/2|5    truncated great dodecahedron\n"
"U38        5/2 5|2    rhombidodecadodecahedron\n"
"U39       2 5/2 5|    small rhombidodecahedron\n"
"U40       |2 5/2 5    snub dodecadodecahedron\n"
"U41        3|5/3 5    ditrigonal dodecadodecahedron\n"
"U42        3 5|5/3    great ditrigonal dodecicosidodecahedron\n"
"U43        5/3 3|5    small ditrigonal dodecicosidodecahedron\n"
"U44        5/3 5|3    icosidodecadodecahedron\n"
"U45       5/3 3 5|    icositruncated dodecadodecahedron\n"
"U46       |5/3 3 5    snub icosidodecadodecahedron\n"
"U47        3/2|3 5    great ditrigonal icosidodecahedron\n"
"U48        3/2 5|3    great icosicosidodecahedron\n"
"U49        3/2 3|5    small icosihemidodecahedron\n"
"U50       3/2 3 5|    small dodecicosahedron\n"
"U51        5/4 5|5    small dodecahemidodecahedron\n"
"U52        3|2 5/2    great stellated dodecahedron\n"
"U53        5/2|2 3    great icosahedron\n"
"U54        2|5/2 3    great icosidodecahedron\n"
"U55        2 5/2|3    great truncated icosahedron\n"
"U56       2 5/2 3|    rhombicosahedron\n"
"U57       |2 5/2 3    great snub icosidodecahedron\n"
"U58        2 5|5/3    small stellated truncated dodecahedron\n"
"U59       5/3 2 5|    truncated dodecadodecahedron\n"
"U60       |5/3 2 5    inverted snub dodecadodecahedron\n"
"U61      5/2 3|5/3    great dodecicosidodecahedron\n"
"U62      5/3 5/2|3    small dodecahemicosahedron\n"
"U63     5/3 5/2 3|    great dodecicosahedron\n"
"U64     |5/3 5/2 3    great snub dodecicosidodecahedron\n"
"U65        5/4 5|3    great dodecahemicosahedron\n"
"U66        2 3|5/3    great stellated truncated dodecahedron\n"
"U67        5/3 3|2    great rhombicosidodecahedron\n"
"U68       5/3 2 3|    great truncated icosidodecahedron\n"
"U69       |5/3 2 3    great inverted snub icosidodecahedron\n"
"U70    5/3 5/2|5/3    great dodecahemidodecahedron\n"
"U71      3/2 3|5/3    great icosihemidodecahedron\n"
"U72   |3/2 3/2 5/2    small retrosnub icosicosidodecahedron\n"
"U73     3/2 5/3 2|    great rhombidodecahedron\n"
"U74     |3/2 5/3 2    great retrosnub icosidodecahedron\n"
"U75  3/2 5/3 3 5/2    great dirhombicosidodecahedron\n"
"U76          2 5|2    pentagonal prism\n"
"U77         |2 2 5    pentagonal antiprism\n"
"U78        2 5/2|2    pentagrammic prism\n"
"U79       |2 2 5/2    pentagrammic antiprism\n"
"U80       |2 2 5/3    pentagrammic crossed antiprism\n";


const char *help_johnson =
"Johnson Polyhedra\n"
"=================\n"
"Johnson polyhedra can be specified by a\n"
"\n"
"o   A J number e.g. j8\n"
"\n"
"o   j_ followed by a name (see the list below) Use '_' instead of\n"
"    a space to avoid having to quote the model name. The beginning\n"
"    of a name can be given and the first match from the list of\n"
"    Johnson polyhedra below is returned.\n"
"\n"
"    When giving a name the following abbreviations can be used\n"
"       tri:   triangular\n"
"       sq:    square\n"
"       squ:   square\n"
"       pe:    pentagonal\n"
"       pen:   pentagonal\n"
"       el:    elongated\n"
"       ge:    gyroelongated\n"
"       tr:    truncated\n"
"       au:    augmented\n"
"       ba:    biaugmenbted\n"
"       ta:    triaugmented\n"
"\n"
"    e.g. j_elongated_triangular_pyramid, j_el_tri_pyr, j_el\n"
"\n"
"\n"
"Johnson List:\n"
"\n"
"J No.  Name\n"
"-----  -------------------------------\n"
"J1     square pyramid\n"
"J2     pentagonal pyramid\n"
"J3     triangular cupola\n"
"J4     square cupola\n"
"J5     pentagonal cupola\n"
"J6     pentagonal rotunda\n"
"J7     elongated triangular pyramid\n"
"J8     elongated square pyramid\n"
"J9     elongated pentagonal pyramid\n"
"J10    gyroelongated square pyramid\n"
"J11    gyroelongated pentagonal pyramid\n"
"J12    triangular dipyramid\n"
"J13    pentagonal dipyramid\n"
"J14    elongated triangular dipyramid\n"
"J15    elongated square dipyramid\n"
"J16    elongated pentagonal dipyramid\n"
"J17    gyroelongated square dipyramid\n"
"J18    elongated triangular cupola\n"
"J19    elongated square cupola\n"
"J20    elongated pentagonal cupola\n"
"J21    elongated pentagonal rotunda\n"
"J22    gyroelongated triangular cupola\n"
"J23    gyroelongated square cupola\n"
"J24    gyroelongated pentagonal cupola\n"
"J25    gyroelongated pentagonal rotunda\n"
"J26    gyrobifastigium\n"
"J27    triangular orthobicupola\n"
"J28    square orthobicupola\n"
"J29    square gyrobicupola\n"
"J30    pentagonal orthobicupola\n"
"J31    pentagonal gyrobicupola\n"
"J32    pentagonal orthocupolarotunda\n"
"J33    pentagonal gyrocupolarotunda\n"
"J34    pentagonal orthobirotunda\n"
"J35    elongated triangular orthobicupola\n"
"J36    elongated triangular gyrobicupola\n"
"J37    elongated square gyrobicupola\n"
"J38    elongated pentagonal orthobicupola\n"
"J39    elongated pentagonal gyrobicupola\n"
"J40    elongated pentagonal orthocupolarotunda\n"
"J41    elongated pentagonal gyrocupolarotunda\n"
"J42    elongated pentagonal orthobirotunda\n"
"J43    elongated pentagonal gyrobirotunda\n"
"J44    gyroelongated triangular bicupola\n"
"J45    gyroelongated square bicupola\n"
"J46    gyroelongated pentagonal bicupola\n"
"J47    gyroelongated pentagonal cupolarotunda\n"
"J48    gyroelongated pentagonal birotunda\n"
"J49    augmented triangular prism\n"
"J50    biaugmented triangular prism\n"
"J51    triaugmented triangular prism\n"
"J52    augmented pentagonal prism\n"
"J53    biaugmented pentagonal prism\n"
"J54    augmented hexagonal prism\n"
"J55    parabiaugmented hexagonal prism\n"
"J56    metabiaugmented hexagonal prism\n"
"J57    triaugmented hexagonal prism\n"
"J58    augmented dodecahedron\n"
"J59    parabiaugmented dodecahedron\n"
"J60    metabiaugmented dodecahedron\n"
"J61    triaugmented dodecahedron\n"
"J62    metabidiminished icosahedron\n"
"J63    tridiminished icosahedron\n"
"J64    augmented tridiminished icosahedron\n"
"J65    augmented truncated tetrahedron\n"
"J66    augmented truncated cube\n"
"J67    biaugmented truncated cube\n"
"J68    augmented truncated dodecahedron\n"
"J69    parabiaugmented truncated dodecahedron\n"
"J70    metabiaugmented truncated dodecahedron\n"
"J71    triaugmented truncated dodecahedron\n"
"J72    gyrate rhombicosidodecahedron\n"
"J73    parabigyrate rhombicosidodecahedron\n"
"J74    metabigyrate rhombicosidodecahedron\n"
"J75    trigyrate rhombicosidodecahedron\n"
"J76    diminished rhombicosidodecahedron\n"
"J77    paragyrate diminished rhombicosidodecahedron\n"
"J78    metagyrate diminished rhombicosidodecahedron\n"
"J79    bigyrate diminished rhombicosidodecahedron\n"
"J80    parabidiminished rhombicosidodecahedron\n"
"J81    metabidiminished rhombicosidodecahedron\n"
"J82    gyrate bidiminished rhombicosidodecahedron\n"
"J83    tridiminished rhombicosidodecahedron\n"
"J84    snub disphenoid\n"
"J85    snub square antiprism\n"
"J86    sphenocorona\n"
"J87    augmented sphenocorona\n"
"J88    sphenomegacorona\n"
"J89    hebesphenomegacorona\n"
"J90    disphenocingulum\n"
"J91    bilunabirotunda\n"
"J92    triangular hebesphenorotunda\n";


const char *help_polygon =
"Polygon-based Polyhedra\n"
"=======================\n"
"Polygon-based polyhedra have unit edges and are specified by their\n"
"abbreviated class name followed by the base polygon, given as a fraction.\n"
"(Note that some models, e.g. pyr7, cannot be made to have unit edges.)\n"
"\n"
"The class names are\n"
"   pri:     prism\n"
"   ant:     antiprism\n"
"   pyr:     pyramid\n"
"   dip:     dipyramid\n"
"   cup:     cupola\n"
"   ort:     orthobicupola\n"
"   gyr:     gyrobicupola\n"
"\n"
"   e.g. pri5, pyr7/2"
"\n";


const char *help_uniform_compounds =
"Uniform Compounds\n"
"=================\n"
"Uniform compounds can be specified by\n"
"\n"
"o   A UC number e.g. uc5\n"
"\n"
"o   uc_ followed by a name (see the list below) Use '_' instead of\n"
"    a space to avoid having to quote the model name. The beginning\n"
"    of a name can be given and the first match from the list of\n"
"    uniform compounds below is returned.\n"
"\n"
"    When giving a name the following abbreviations can be used\n"
"       tr:     truncated\n"
"       sm:     small\n"
"       gr:     great\n"
"       st:     stellated\n"
"       sn:     snub\n"
"       tet:    tetrahedra\n"
"       ico:    icosahedra\n"
"       icosa:  icosahedra\n"
"       dod:    dodecahedra\n"
"       oct:    octahedra\n"
"       cubo:   cuboctahedra\n"
"       icosid: icosidodecahedra\n"
"       pri:    prisms\n"
"       ant:    antiprisms\n"
"       rot:    rotational\n"
"\n"
"    e.g. uc_5_tetrahedra, uc_5_tet, uc_2_tr_tet, uc_2_tr_t\n"
"\n"
"    When a compound is listed as rotational, an angle can be supplied after\n"
"    an underscore. If no angle is supplied, a random angle is generated\n"
"\n"
"    e.g. uc2_a30, uc7_a22.5, uc11_a11.315, uc28_a0\n"
"\n"
"    For uc20 to uc25 n/d and k can be supplied after an underscore.\n"
"    If no n/d or k is supplied then random values are generated\n"
"\n"
"    e.g. uc21_n5/2k4, uc22_n7/3, uc23_n7/3k3, uc25_n7/4k3\n"
"\n"
"    Because uc20, uc22, and uc24 an angle can also be supplied.\n"
"    Note that the order of a, n/d and k does not matter\n"
"\n"
"    e.g. uc20_n9/2, uc20_k5, uc22_n7/3k2a4, uc22_a7n5/3k3, uc24_a3k5n7/4\n"
"\n"
"\n"
"Uniform Compound List:\n"
"\n"
"UC No  Name\n"
"-----  -------------------------------\n"
"UC1    6 tetrahedra rotational\n"
"UC2    12 tetrahedra rotational\n"
"UC3    6 tetrahedra\n"
"UC4    2 tetrahedra\n"
"UC5    5 tetrahedra\n"
"UC6    10 tetrahedra\n"
"UC7    6 cubes rotational\n"
"UC8    3 cubes\n"
"UC9    5 cubes\n"
"UC10   4 octahedra rotational\n"
"UC11   8 octahedra rotational\n"
"UC12   4 octahedra\n"
"UC13   20 octahedra rotational\n"
"UC14   20 octahedra\n"
"UC15   10 octahedra 1\n"
"UC16   10 octahedra 2\n"
"UC17   5 octahedra\n"
"UC18   5 tetrahemihexahedra\n"
"UC19   20 tetrahemihexahedra\n"
"UC20   2k n d gonal prisms rotational\n"
"UC21   k n d gonal prisms\n"
"UC22   2k n odd d gonal antiprisms rotational\n"
"UC23   k n odd d gonal antiprisms\n"
"UC24   2k n even d gonal antiprisms rotational\n"
"UC25   k n even d gonal antiprisms\n"
"UC26   12 pentagonal antiprisms rotational\n"
"UC27   6 pentagonal antiprisms\n"
"UC28   12 pentagrammic crossed antiprisms rotational\n"
"UC29   6 pentagrammic crossed antiprisms\n"
"UC30   4 triangular prisms\n"
"UC31   8 triangular prisms\n"
"UC32   10 triangular prisms\n"
"UC33   20 triangular prisms\n"
"UC34   6 pentagonal prisms\n"
"UC35   12 pentagonal prisms\n"
"UC36   6 pentagrammic prisms\n"
"UC37   12 pentagrammic prisms\n"
"UC38   4 hexagonal prisms\n"
"UC39   10 hexagonal prisms\n"
"UC40   6 decagonal prisms\n"
"UC41   6 decagrammic prisms\n"
"UC42   3 square antiprisms\n"
"UC43   6 square antiprisms\n"
"UC44   6 pentagrammic antiprisms\n"
"UC45   12 pentagrammic antiprisms\n"
"UC46   2 icosahedra\n"
"UC47   5 icosahedra\n"
"UC48   2 great dodecahedra\n"
"UC49   5 great dodecahedra\n"
"UC50   2 small stellated dodecahedra\n"
"UC51   5 small stellated dodecahedra\n"
"UC52   2 great icosahedra\n"
"UC53   5 great icosahedra\n"
"UC54   2 truncated tetrahedra\n"
"UC55   5 truncated tetrahedra\n"
"UC56   10 truncated tetrahedra\n"
"UC57   5 truncated cubes\n"
"UC58   5 stellated truncated hexahedra\n"
"UC59   5 cuboctahedra\n"
"UC60   5 cubohemioctahedra\n"
"UC61   5 octahemioctahedra\n"
"UC62   5 rhombicuboctahedra\n"
"UC63   5 small rhombihexahedra\n"
"UC64   5 small cubicuboctahedra\n"
"UC65   5 great cubicuboctahedra\n"
"UC66   5 great rhombihexahedra\n"
"UC67   5 great rhombicuboctahedra\n"
"UC68   2 snub cubes\n"
"UC69   2 snub dodecahedra\n"
"UC70   2 great snub icosidodecahedra\n"
"UC71   2 great inverted snub icosidodecahedra\n"
"UC72   2 great retrosnub icosidodecahedra\n"
"UC73   2 snub dodecadodecahedra\n"
"UC74   2 inverted snub dodecadodecahedra\n"
"UC75   2 snub icosidodecadodecahedra\n";

const char *help_color_map =
"Colour Maps\n"
"===========\n"
"OFF elements may be coloured by index numbers. Colour maps provide a\n"
"way to convert these index numbers into colour values and are used\n"
"in command options such as off_color -m, antiview -m and n_icons -M.\n"
"\n"
"The maps may use the Antiprism colour map format, Gimp Palette format\n"
"or Fractint format. The Antiprism format has lines of the form\n"
"index_number = color_value # comment_text', e.g. '2 = 1.0,0.0,0.0 # red'\n"
"anything after # is a comment and ignored. Blank lines are ignored\n"
"\n"
"A map may be given by several maps separated by ',' e.g. 'map1,map2'.\n"
"The maps are tried in order until a conversion is found for an index\n"
"number\n"
"\n"
"Map modifiers\n"
"-------------\n"
"A map may be modified by remapping its own entries. The modifiers\n"
"can be given in any order but the general form is\n"
"\n"
"   map_name+shift*step%%wrap\n"
"\n"
"Any index, idx, is mapped to the colour value with index\n"
"   wrap is 0:      shift + idx*step\n"
"   wrap is not 0: (shift + idx*step) %% wrap   [where %% is modulus]\n"
"The defaults are shift=0, step=1, wrap=0\n"
"e.g. cmap+1  : for index 10 get the colour value for cmap index 11\n"
"     cmap*2  : for index 10 get the colour value for cmap index 20\n"
"     cmap%%6 : for index 10 get the colour value for cmap index 4\n"
"\n"
"Resource Maps\n"
"-------------\n"
"Internal (see below for format):\n"
"   spread\n"
"      Gives a range of colours each differing from the last few. Useful\n"
"      to colour elements whose index numbers have been set sequentially.\n"
"   rnd, rand, random\n"
"      A random map, with colours selected within certain ranges\n"
"      (default: component ranges H0:1S0.7:1V0.7:1).\n"
"   rng, range\n"
"      A map made by ranging between component values\n"
"      (default: size 256, component ranges H0:1S0.9V0.9).\n"
"   remap\n"
"      A map of index numbers to themselves. Use with the map modifiers\n"
"      to remap index numbers.\n"
"   null\n"
"      An empty map.\n"
"   grey, greyw\n"
"      greyscales (default: size 256), grey runs from balck to white\n"
"      and greyw is wrappable and runs from black to white to black again.\n"
"   uniform\n"
"      used to colour the uniform, Johnson and polygon-based resource\n"
"      models (applied with off_color -f A -m uniform)\n"
"   compound\n"
"      used to colour the uniform compound resource models (applied\n"
"      with off_color -f K -v F -e F -m compound)\n"
"\n"
"   These maps contain a mapping for every index number (except default for\n"
"   rng is 256 entries). Follow the map name immediately by a number to set\n"
"   a particular size for the map (e.g. rnd64). The random and range maps\n"
"   (with the optional size specification) can be followed by '_' and then\n"
"   a letter from HSVA or RGBA to specify a component. This is followed\n"
"   by floating point numbers numbers separated by ':'. The random map\n"
"   allows one number for a fixed value, or two numbers for a range. The\n"
"   range map places the components at equal steps throughout the map\n"
"   and interpolates between the values\n"
"   e.g. rnd - default random map\n"
"        rnd64 - random map with 64 entries\n"
"        rnd256_S0V0:1 - random map with 256 grey entries\n"
"        rng - default range map, like a rainbow map with 256 entries\n"
"        rng16 - rainbow map with 16 entries\n"
"        rng_S0V0:1 - greyscale with 256 entries\n"
"        rng_R0:1:1G1:1:0B1:0:1 - runs from cyan to yellow to magenta.\n"
"\n"
"External (in resource directory 'col_maps'):\n"
"   x11 (549 colours)\n"
"       Broad range of named colours from X11's rgb.txt. The names may\n"
"       also be used to specify colours on the command line.\n"
"   vga (16 colours)\n"
"       Original VGA named colours.\n"
"   html (140 colours)\n"
"       Broad range of colours. These are the colours that can be used\n"
"       by name in HTML.\n"
"   ms (48 colours)\n"
"       A color map based on the Micosoft color dialog.\n"
"   iscc (267 colours)\n"
"       Colour centroids (http://tx4.us/nbs-iscc.htm)\n"
"   rainbow (224 colours)\n"
"       A rainbow map, with cyan and green\n"
"   rainbowc (192 colours)\n"
"       A rainbow map, with cyan but not green\n"
"   rainbowg (192 colours)\n"
"       A rainbow map, with green but not cyan\n"
"   spectrum (401 colours)\n"
"       An approximate visible spectrum\n"
;
