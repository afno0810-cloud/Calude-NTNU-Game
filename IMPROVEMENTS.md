# Forbedringer i Gløshaugen Strike

Andreas' mål: minst 1000 forbedringer. Hver linje under er én konkret ting som er lagt til eller gjort bedre.
Nyeste runde øverst.

## Runde 4: grafikk (25. nov. kveld)

### Materialer og teksturer
1. Egen PBR-tekstur for rød murstein (farge, relieff, ruhet, skygger i fugene)
2. Brun murstein (Elektrobygget)
3. Gul murstein (Realfagbygget)
4. Grå granitt i kvader med ujevne skift (Hovedbygningen)
5. Mørk sokkelstein
6. Lys sandstein til gesimser, vindusbrett og bånd
7. Skifertak med overlappende skifer og litt mose
8. Puss med fine ujevnheter og regnstriper
9. Forskalingsbetong med bordstruktur og porer
10. Glatt betong
11. Eikeparkett
12. Mørkt treverk
13. Marmorfliser med årer
14. Terrazzogulv
15. Kobber med irr
16. Børstet metall med riper
17. Lakkert metall
18. Vevd stoff
19. Asfalt med steinkorn og sprekker
20. Belegningsstein med mose i fugene
21. Gress med tørre flekker og jord
22. Grus
23. Plast
24. Gummi
25. Løvatlas med gjennomskinnelige blader
26. Furu-/lønnebark
27. Bjørkebark
28. Stor variasjonstekstur som skjuler at teksturene gjentar seg
29. Alle teksturene er sømløse
30. Unreal lager hovedmaterialet M_GS_Surface automatisk (farge × tone × variasjon, normal, AO, ruhet, metall)
31. Verdensprojisert materiale for bakke og veier, så teksturene ikke strekkes
32. Ekte vindusglass: gjennomsiktig, reflekterende og tosidig
33. Løvmateriale med maskering, tosidig lys gjennom bladene og vind
34. 60 ferdige materialinstanser (MI_GS_*)
35. Materialer legges automatisk på alle modeller etter navn på materialsporet
36. Skannede materialer (Poly Haven, Megascans, Fab) vinner automatisk over våre når navnet passer
37. Søkeord for Poly Haven-navn (red_brick, castle_wall, asphalt, aerial_grass ...)
38. 7 billakkfarger (sølv, sort, hvit, rød, blå, grå, grønn)

### Lys og rendering
39. Hardware ray tracing i Lumen (RTX), både lys og refleksjoner
40. Treffbasert lys i refleksjoner, så speilinger viser riktige farger
41. Refleksjoner i glass (front layer)
42. Konsollkommando `Straaling 0/1` for å slå ray tracing av og på
43. Kartet `Gloshaugen` lages automatisk første gang
44. Kartet har sol, himmel, skylys, tåke og skyer, så det ser riktig ut i editoren også

### Bygg
45. Gamle fysikk som ekte 3D-modell
46. Gamle kjemi som ekte 3D-modell
47. Kjelhuset som ekte 3D-modell
48. Kjelhuspipa: 38 m teglpipe med jernbånd
49. Sentralbygg 1 som ekte 3D-modell
50. Sentralbygg 2 som ekte 3D-modell
51. Bergbygget som ekte 3D-modell
52. Elektrobygget som ekte 3D-modell
53. IT-bygget som ekte 3D-modell
54. Stripa med glassfasade
55. Kjemiblokkene som ekte 3D-modell
56. Varmeteknisk som ekte 3D-modell
57. Realfagbygget som ekte 3D-modell
58. Idrettsbygget med glassfasade
59. Hovedbygningen vest som ekte 3D-modell
60. Hovedbygningen øst som ekte 3D-modell
61. Vindusrammer med midtpost og tverrpost i alle vinduer
62. Glassruter i alle vinduer, uten kollisjon, så du kan skyte og hoppe gjennom
63. Vindusbrett under alle vinduer
64. Overligger over vinduene på de klassiske byggene
65. Sokkel langs bunnen av alle bygg
66. Etasjebånd på de klassiske byggene og på betongbyggene
67. Gesims øverst på de klassiske byggene
68. Saltak i skifer på de gamle byggene
69. Kobberkam på mønet
70. Takarker med vinduer
71. Piper på takene
72. Flate tak med takpapp og grus
73. Brystning med beslag rundt flate tak
74. Maskinrom på taket over trappa
75. Ventilasjonsaggregater med vifter på takene
76. Takvinduer
77. Lufterør
78. Antenner og rødt flylys på de høye byggene
79. Vertikale aluminiumsribber på glassbyggene
80. Takutstikk på glassbyggene
81. Dørkarmer og glass over dørene
82. Baldakin over alle innganger
83. Lampe over hver inngang
84. Dørmatte ved hver inngang
85. Skilt over hovedinngangen
86. Puss på innsiden av alle yttervegger
87. Terrazzogulv i alle etasjer
88. Takplater med lys i taket i alle etasjer
89. Rekkverk langs rampene
90. Rekkverk rundt trappeåpningen
91. Hovedbygningen: vindusrammer i alle vinduer
92. Hovedbygningen: glass i alle vinduer
93. Hovedbygningen: glasstak over Lysgården
94. Mørkere og mer riktig stein på Hovedbygningen

### Ute
95. Gress med ekte tekstur på hele campus
96. Asfalt på veiene
97. Belegningsstein på gangstiene
98. Fortau langs alle bilveier
99. Kantstein
100. Stiplet midtlinje
101. Heltrukne kantlinjer
102. Fortau og linjer stopper i kryss og ved byggvegger
103. Lønnetre
104. Bjørk
105. Stor gran
106. Liten gran
107. Busker
108. Klippede hekker langs byggene
109. 220 trær i stedet for 160
110. Trær har usynlig stammekollisjon, så kuler går gjennom løvet, men ikke stammen
111. Sedan
112. Kombi
113. SUV med takrails
114. Liten bil
115. Varebil
116. Bilene har hjul med felger, lys, skilt, speil, dørhåndtak og støtfangere
117. Bilene står parkert ved fortauskanten, ikke oppå den
118. 60 biler i stedet for 36, uten at de står oppå hverandre eller i kryss

### Mennesker og lys
119. Andre spillere er ekte animerte mennesker (Unreal-mannekengen Manny/Quinn) i stedet for sylindre, når Third Person-pakken er i prosjektet
120. Manny og Quinn fordeles mellom spillerne
121. Ekte ragdoll når noen dør
122. Sylinder og kule er igjen som usynlige treffbokser, så treff virker som før
123. Lyspaneler i taket i alle etasjer i alle bygg som lyser opp rommene med Lumen (og om natta)
124. Lys over hver inngang
125. PC-skjermer lyser
126. Lysekronene i Lysgården lyser
127. Ekte gatelykter med stolpe, arm og lampehode over veien
128. Gatelyset kommer fra lampehodet, ikke fra stolpen

### Vær på overflatene
129. Våte gater og vegger når det regner: alt blir mørkere og blankt, og ray tracing gir speilinger i asfalten
130. Det tar omtrent ett minutt med regn før alt er vått, og noen minutter å tørke opp igjen
131. Snø legger seg på alt som vender opp (tak, bakke, benker, biler) når det snør
132. Snøen smelter sakte bort etter at det har sluttet å snø

### Ytelse
133. Møbler og ting ute bruker hierarkisk instansiering (HISM), så de som ikke synes tegnes ikke
134. Trær, busker og biler bruker også HISM med avstandsgrense
135. Ingen .glb-forhåndsvisninger av store modeller i repoet (mindre nedlasting)

## Runde 3 (25. nov. ettermiddag)
136. Hovedbygningen som ekte 3D-modell med Lysgården, trapp, scene og gallerier
137. Tårnet med spiralrampe opp til klokketårnet
138. 43 møbler, utendørsting og roboter
139. Møbler i alle rom: klasserom, kontor, lab, kantine, bibliotek, salong, serverrom, lager
140. Benker, sykkelstativ, sparkesykler, busskur, søppelkasser, containere, piknikbord, byggegjerder, NTNU-skilt
141. Robothund, vaktrobot og leveringsrobot
142. PC-er med overvåkningskamera
143. Sporing av alle spillere på kart fra PC
144. 3D-printere som lager 13 ting av filament
145. 38 overvåkningskameraer
146. Ammo per kaliber (15 kalibre)
147. Panserbrytende ammo
148. Hulspissammo
149. Brannammo
150. Sporlysammo
151. Sjokkammo
152. Holo-sikte
153. 8x kikkertsikte
154. Trommelmagasin
155. Kompensator
156. Grep
157. Modifikasjoner som erstatter hverandre i samme spor
158. Over/under-hagle
159. M72 LAW (engangs)
160. Mosin
161. Remington 700
162. M14
163. G36
164. SCAR
165. UMP
166. P90
167. Vector
168. Signalpistol
169. Langbue
170. Printet pistol
171. Tobladet vikingøks
172. Spyd
173. Printet kniv
174. Printet hjelm
175. Ladeanimasjon med magasin ut, magasin inn og sleide
176. Patron-for-patron lading på hagler
177. Magasinet faller ut og blir liggende
178. Inspisering av våpenet (T)
179. Pust, skritt og sprint-stilling
180. Rekyl med fjær
181. Nærkamp med oppsving og slag
182. Lefse, fårikål, pinnekjøtt, julebrus, risgrøt, tran, Kvikk Lunsj, solbærtoddy, rakfisk og sodd
183. Volumetriske skyer
184. Fargekorrigering etter vær og tid på døgnet
185. Konsollkommandoene `Grafikk` og `Opplosning`
186. Treteksturer på kolber og skjefter
187. Stål-, krom- og blåneringsmaterialer på løp og sleider
188. Messing og gull på patroner og pyntevåpen
189. Polymer, bakelitt og gummi på grep
190. Oliven, khaki og tan på militærutstyr
191. Ny skinntekstur for hylstre, remmer og vester
192. Viklet håndtak på katanaen
193. Horisont: 420 hus og blokker rundt campus
194. Skogkledde åser i en ring 1,3 til 4 km ute
195. Fjorden i nord
196. Fjell og jorder som skifter etter høyde
197. Kumlokk i støpejern i kjørebanene
198. Sluk med rist langs fortauskanten
199. Røde brannhydranter langs fortauene
200. Grå koblingsskap for strøm
201. 70 morenesteiner på plenene (gir dekning)
202. Tre steinstørrelser med flate bruddflater
203. 30 runde blomsterbed med steinkant
204. Blomster i rødt, gult, hvitt og blått

*(Rundene før dette: 43 våpen, sjeldenhet, overlevelse, boosts, rustning, granater og utstyr, loot, vær og dag/natt; se README.)*
