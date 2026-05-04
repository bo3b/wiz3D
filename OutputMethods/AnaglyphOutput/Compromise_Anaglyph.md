# Introducing a new anaglyph method: compromise anaglyph

**Jure Ahtik**
University of Ljubljana, Faculty of Natural Sciences and Engineering
Chair of Information and Graphic Technology
Snežniška 5, SI-1000 Ljubljana, Slovenia
E-mail: jure.ahtik@ntf.uni-lj.si

### Abstract
Anaglyph photography is one of the stereo photographic techniques. Anaglyphs consists of two stereo pairs, which are based on complementary colour separations. The most known and used pair is red-cyan. Main advantage of anaglyph method is, that it can be rendered using any presentation technique. Different methods of making digital anaglyphs can be found, and each of them has its advantages and disadvantages. We decided to compare true, grey, colour, half-colour, Dubois and optimised anaglyph techniques and to develop improvements to the process of making anaglyphs.

**Keywords:** anaglyph, stereo photography, compromise anaglyph, conversion matrix, colour difference

---

### 1. Introduction
Stereo photography is one of the photographic techniques, with the possibility to influence photography, videography and digital media in the future the most. Some of stereographic methods are almost as old as photography itself and one of them is called anaglyph photography.

Stereo photography is based on capturing images that can be seen with each human eye separately in the same moment, combining them into one stereo image and looking at them in a way, that only one stereo pair can be seen with each eye. Anaglyphs are made of two stereo pairs, which are based on complementary colour separations (Valzus, 1966). The most known and used pair is red-cyan, but there are also others, such as amber-blue. Main advantage of anaglyph method is, that it can be rendered using any presentation technique, and it can be printed and used on a paper and certainly the best one for doing so. The other great advantage is its price, because no electronic equipment is needed for its use and distribution (Vierling, 1965).

Procedure how to make an anaglyph is, first capture photographs of both stereo pairs, than to make colour separations out of them, and last, to combine them into a final anaglyph. Using colour filters, that are usually fitted in a special pair of glasses, healthy human can see stereo effect. There are different techniques of how to make a red-cyan anaglyph using different conversion matrices in the process (Wimmer, 2010).

The research was based on comparison of different known techniques of producing anaglyphs using colour matrices, colour measurement and transformations, visual evaluation and on development of possible improvements (Ahtik, 2011).

---

### 2. Methods

#### 2.1. Measuring colour filters
First we needed to analyse the colour filtered anaglyph glasses that are available on the market. Using spectrophotometer X-rite EyeOne Pro, series of spectral measurements of light emitting through four samples of red-cyan filters, were performed. Converting spectral measurements to CIEXYZ and CIELAB colour spaces, colour differences ΔE*ab between average and single CIELAB values were calculated.

#### 2.2. Making of stereo pairs
Digital photographies of stereo pairs had been made with Nikon D700 digital camera and AF-S NIKKOR 24-70mm f/2.8G ED lens. ColorChecker test chart fields (Gardner, 2011) were embedded into stereopairs for better colour comparison and measurements. *(See Figure 1 in original text).*

#### 2.3. Comparing known conversion methods
The next step in the research was to compare different techniques of making anaglyphs. All colour conversions in the research had been calculated with conversion matrices, one matrix per one colour separation. This technique works with digital RGB images, where we multiply image, as an 3-by-1 matrix, with 3-by-3 conversion matrix, effecting each RGB separation on all three RGB channels. That way the very precise conversion is possible. 

For each of the conversion techniques, pair of conversion matrices is known: true anaglyph [1], grey anaglyph [2], colour anaglyph [3], half-colour anaglyph [4], Dubois anaglyph [5] and optimised anaglyph [6] (Dubois, 2010&2011; Glesson, 2010; Wimmer, 2010).

*Where:* * `r_a, g_a, b_a` = red, green and blue values of the final anaglyph image
* `r_1, g_1, b_1` = red, green and blue values of the first stereopair (left eye)
* `r_2, g_2, b_2` = red, green and blue values of the second stereopair (right eye)

**[1] True Anaglyph**
```
[r_a]   [ 0,299  0,587  0,114 ]   [r_1]   [ 0      0      0     ]   [r_2]
[g_a] = [ 0      0      0     ] * [g_1] + [ 0      0      0     ] * [g_2]
[b_a]   [ 0      0      0     ]   [b_1]   [ 0,299  0,587  0,114 ]   [b_2]
```

**[2] Grey Anaglyph**
```
[r_a]   [ 0,299  0,587  0,114 ]   [r_1]   [ 0      0      0     ]   [r_2]
[g_a] = [ 0      0      0     ] * [g_1] + [ 0,299  0,587  0,114 ] * [g_2]
[b_a]   [ 0      0      0     ]   [b_1]   [ 0,299  0,587  0,114 ]   [b_2]
```

**[3] Colour Anaglyph**
```
[r_a]   [ 1  0  0 ]   [r_1]   [ 0  0  0 ]   [r_2]
[g_a] = [ 0  0  0 ] * [g_1] + [ 0  1  0 ] * [g_2]
[b_a]   [ 0  0  0 ]   [b_1]   [ 0  0  1 ]   [b_2]
```

**[4] Half-colour Anaglyph**
```
[r_a]   [ 0,299  0,587  0,114 ]   [r_1]   [ 0  0  0 ]   [r_2]
[g_a] = [ 0      0      0     ] * [g_1] + [ 0  1  0 ] * [g_2]
[b_a]   [ 0      0      0     ]   [b_1]   [ 0  0  1 ]   [b_2]
```

**[5] Dubois Anaglyph**
```
[r_a]   [  0,456   0,500   0,176 ]   [r_1]   [ -0,043  -0,088  -0,002 ]   [r_2]
[g_a] = [ -0,040  -0,038  -0,016 ] * [g_1] + [  0,378   0,734   0,018 ] * [g_2]
[b_a]   [ -0,015  -0,021  -0,016 ]   [b_1]   [ -0,072  -0,113   1,226 ]   [b_2]
```

**[6] Optimised Anaglyph**
```
[r_a]   [ 0  0,7  0,3 ]   [r_1]   [ 0  0  0 ]   [r_2]
[g_a] = [ 0  0    0   ] * [g_1] + [ 0  1  0 ] * [g_2]
[b_a]   [ 0  0    0   ]   [b_1]   [ 0  0  1 ]   [b_2]
```

Using Adobe RGB to CIEXYZ and CIEXYZ to CIELAB colour conversions, CIELAB values of ColorChecker test chart fields were calculated. In the area of stereo photography judgement cannot be based on a colour measurements that can be compared to normal human vision, only. Good anaglyph is a synthesis of two things: good colour reproduction and good stereo effect. According to this, visual comparison of all known techniques has been made and two parameters were observed: quality of stereo effect and ghosting. Stereo effect represents quality of three-dimensional presentation and ghosting represents an effect, when both stereo pairs can be seen at the same moment through the same filter, producing double image. The proper way of seeing anaglyphs is just one image through one corresponding filter. Visual evaluation has been made on LCD screen Eizo CE240, LED screen on Apple MacBook Pro and on digital print made with Canon imagePRESS C1+.

#### 2.4. Creating a new conversion method
New conversion methods had been developed with calculating average conversion matrix out of different know conversion matrices and with adjusting them.

---

### 3. Results and discussion

#### 3.1. Measurements of colour filters
Measuring all four filter samples has shown a great similarity between them. Calculated colour differences ΔE*ab between average and single CIELAB values were calculated and they are all lower than 6.49. Based on the analysis of the measurements, conclusion has been made, that colour differences between different colour filters are not high enough to influence stereo effect dramatically. We believe that colour filter adaptation or correction is not needed in the process of making colour separations. Observation has been made, that anaglyph technique is indeed very suitable for wide market use, mainly because is easy to produce anaglyph glasses.

#### Emission Spectra (Approximated from Figure)

| Wavelength (nm) | White Light (cd/m²) | Cyan Filter Avg (cd/m²) | Red Filter Avg (cd/m²) |
|-----------------|---------------------|--------------------------|------------------------|
| 380             | 0                   | 0                        | 0                      |
| 400             | 60                  | 25                       | 0                      |
| 420             | 20                  | 10                       | 2                      |
| 440             | 330                 | 220                      | 5                      |
| 460             | 180                 | 140                      | 0                      |
| 480             | 220                 | 180                      | 0                      |
| 500             | 235                 | 185                      | 0                      |
| 520             | 210                 | 150                      | 0                      |
| 540             | 200                 | 120                      | 0                      |
| 560             | 290                 | 150                      | 5                      |
| 580             | 180                 | 40                       | 20                     |
| 600             | 210                 | 15                       | 100                    |
| 620             | 200                 | 5                        | 155                    |
| 640             | 180                 | 5                        | 150                    |
| 660             | 150                 | 3                        | 130                    |
| 680             | 120                 | 2                        | 100                    |
| 700             | 90                  | 3                        | 75                     |
| 720             | 60                  | 5                        | 55                     |
| 730             | 50                  | 10                       | 45                     |

#### 3.2. Comparison of known conversion methods
Application of different conversion matrices on prepared stereo pairs, gave us a results that can be seen on Figures 3 to 8. ColorChecker test chart has been used to measure the results. The average ΔE*ab between original and anaglyph ColorChecker test chart values were calculated and the results showed, that colour anaglyph, with ΔE*ab value 0.00 was far the best, following by half-colour anaglyph 12.09, Dubois anaglyph 17.64, optimised anaglyph 19.38, grey anaglyph 33.69 and true anaglyph 77.21.

The analysis of visual comparison of different techniques showed, that the best results are produced with grey anaglyph, Dubois anaglyph and optimised anaglyph conversion techniques. On the other hand, colour anaglyph, half colour anaglyph and true anaglyph gave worst results - colour anaglyph especially.

#### 3.3. Creation of a new conversion method
In the experiment, trying to develop better way of creating anaglyphs, an average of all conversion matrices has been made [7], producing so called experimental average anaglyph. Calculated average colour difference for ColorChecker test chart fields was 24.08 and visual evaluation was concluded as an average one.

**[7] Average Anaglyph Matrix**
```
[r_a]   [  0,392   0,494   0,136 ]   [r_1]   [ -0,007  -0,015  0,000 ]   [r_2]
[g_a] = [ -0,007  -0,006  -0,003 ] * [g_1] + [  0,113   0,720  0,016 ] * [g_2]
[b_a]   [ -0,003  -0,004  -0,003 ]   [b_1]   [  0,088   0,177  0,742 ]   [b_2]
```

The next step was to exclude two of the worst conversion techniques, according to colour difference. This time an anaglyph has been made with average conversion matrices calculated from matrices for producing colour anaglyph, half-colour anaglyph, Dubois anaglyph and optimised anaglyph. For the purpose of preserving grey shades in the final result, the red component values in cyan conversion matrix were corrected to zero values [8]. The reason for making so was to preserve complementary channels in each colour separation intact. Only doing so, producing perfect reproduction of grey shades when combining both, red and cyan colour separations, is possible. 

**[8] Compromise Anaglyph Matrix**
```
[r_a]   [ 0,439  0,447  0,148 ]   [r_1]   [  0       0       0     ]   [r_2]
[g_a] = [ 0      0      0     ] * [g_1] + [  0,095   0,934  -0,005 ] * [g_2]
[b_a]   [ 0      0      0     ]   [b_1]   [ -0,018  -0,028   1,057 ]   [b_2]
```

Calculated colour difference of 11.53 is however still very high difference, but it is compensated by the good result from the visual evaluation. Visual evaluation, which has been made on LCD screen, LED screen and on digital print, was the best, even better than Dubois anaglyph and optimised anaglyph.

---

### 4. Conclusion
The development of the new, better way for accomplishing something as old as an anaglyph, was not expected, but with a help of new technologies it was not surprising. Used developed methods were not only mathematic but they also involved precise handmade corrections of the equations. Colour measurement and visual evaluation showed better result in comparison to other known techniques. 

The reason, why compromise anaglyph is better than other methods is in a perfect combination of viewing anaglyphs with or without anaglyph glasses. This is very important because anaglyph has to attract viewer while he is not wearing anaglyph glasses and it has to offer good stereo effect for viewers satisfaction. Having that in mind we can conclude that, this research and development of new anaglyph method is a success. Name "compromise anaglyph" has been given to the new developed technique for producing digital colour anaglyphs.

Based on correlation analysis between both comparisons (colour differences and visual evaluation) it has been concluded, that Dubois and optimised anaglyphs are the best techniques of producing good anaglyphs. Those two techniques are also most recent and most developed ones.

---

### References
* Ahtik, J., Techniques of Rendering Anaglyphs for Use in Art - Master thesis. Ljubljana : University of Ljubljana, Faculty of Natural Sciences and Engineering, 2011.
* Dubois, E., Generation of anaglyph stereoscopic images. School of Information Technology and Engineering, University of Ottawa, Ottawa, Canada, 2000.
* Dubois, E., A projection method to generate anaglyph stereo images. IEEE International Conference on Acoustics, Speech, and Signal Processing, Proceedings. (ICASSP '01). Salt Lake City, UT, ZDA, 2001, Vol. 3, pp. 1661-1664.
* Gardner, R., GretagMacbeth Color Checker Numeric Values [online]. <http://www.rags-int-inc.com/PhotoTechStuff/MacbethTarget/>, 10 January 2011.
* Gleeson, S., Wimmer's Optimised Anaglyph on Club Penguin [online]. <http://www.swell3d.com/2008/07/wimmers-optimised-anaglyph-on.html>, 27 December 2010.
* Gleeson, S., Color anaglyph methods compared [online]. <http://www.swell3d.com/color-anaglyph-methods-compare.html>, 27 December 2010.
* Lindbloom, J. B., RGB/XYZ Matrices [online]. <http://www.brucelindbloom.com/>, 21 October 2009.
* Valzus, N. A., Stereoscopy. London : The Focal Press, 1966.
* Vierling, O., Die Stereoskopie in der Photographie und Kinematographie. Stuttgart : Wissenschaftliche Verlagsgesellschaft Mbh., 1965.
* Wimmer, P., Grundlagen der Stereoskopie [online]. <http://pwimmer.gmxhome.de>, 27. 12. 2010