#!/usr/bin/env python
'''CanberraUAV utility functions for dealing with image regions'''

import cuav_util, cv, math
import numpy as np

#########################
### Priority Enumeration
#########################
COLOR_RED  = (255,0,0)
COLOR_YELLOW = (255,255,0)
COLOR_GREEN = (124,252,0)
COLOR_BLUE = (0,0,255)

REGION_VERY_IMPORTANT = COLOR_RED
REGION_IMPORTANT = COLOR_YELLOW
REGION_SOMEWHAT_IMPORTANT = COLOR_GREEN
REGION_NOT_IMPORTANT = COLOR_BLUE

MIN_DEDUPE_REGIONS = 5
DDUPE_ACCURACY_BY = 1

REGION_MERGE_PIXEL_DISTANCE = 20

class Region:
    '''a object representing a recognised region in an image'''
    def __init__(self, x1, y1, x2, y2, scan_shape, scan_score=0, compactness=0, avg_score=0, rarity=0, fg_rarity=0):
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2
        self.latlon = None
        self.score = int(scan_score)
        self.dupe = False
        self.scan_score = scan_score
        self.avg_score = avg_score
        self.rarity = rarity
        self.fg_rarity = fg_rarity
        self.active = True
        self.priority = REGION_SOMEWHAT_IMPORTANT
        self.compactness = compactness
        self.whiteness = None
        self.blue_score = None
        self.scan_shape = scan_shape

    def tuple(self):
        '''return the boundary as a tuple'''
        return (self.x1, self.y1, self.x2, self.y2)

    def center(self):
        '''return the center of the region'''
        return ((self.x1+self.x2)//2, (self.y1+self.y2)//2)

    def draw_rectangle(self, img, colour=(255,0,0), linewidth=2, offset=2):
        '''draw a rectange around the region in an image'''
        (r,g,b) = colour
        (x1,y1,x2,y2) = self.tuple()
        (wview,hview) = cuav_util.image_shape(img)
        (w,h) = self.scan_shape
        x1 = x1*wview//w
        x2 = x2*wview//w
        y1 = y1*hview//h
        y2 = y2*hview//h
        cv.Rectangle(img, (max(x1-offset,0),max(y1-offset,0)), (x2+offset,y2+offset), (b,g,r), linewidth)

    def __str__(self):
        return '%s latlon=%s score=%s' % (str(self.tuple()), str(self.latlon), self.score)
	    
def RegionsConvert(rlist, scan_shape, full_shape, calculate_compactness=False):
    '''convert a region list from tuple to Region format,
	also mapping to the shape of the full image'''
    regions = []
    scan_w = scan_shape[0]
    scan_h = scan_shape[1]
    full_w = full_shape[0]
    full_h = full_shape[1]
    for r in rlist:
        (x1,y1,x2,y2,score,avg_pixel,rarity,fg_rarity) = r
        x1 = int(x1 * full_w) // scan_w
        x2 = int(x2 * full_w) // scan_w
        y1 = int(y1 * full_h) // scan_h
        y2 = int(y2 * full_h) // scan_h
        if calculate_compactness:
            compactness = 0 #array_compactness(pixscore)
        else:
            compactness = 0
        regions.append(Region(x1,y1,x2,y2, scan_shape, score, compactness, avg_pixel, rarity, fg_rarity))
    return regions

def array_compactness(im):
    '''
    calculate the compactness of a 2D array. Each element of the 2D array
    should be proportional to the score of that pixel in the overall scoring scheme
    . '''
    from numpy import array,meshgrid,arange,shape,mean,zeros
    from numpy import outer,sum,max,linalg
    from numpy import sqrt
    from math import exp
    (h,w) = shape(im)
    # make sure we don't try to process really big arrays, as the CPU cost
    # rises very rapidly
    maxsize = 15
    if h > maxsize or w > maxsize:
            reduction_h = (h+(maxsize-1))//maxsize
            reduction_w = (w+(maxsize-1))//maxsize
            im = im[::reduction_h, ::reduction_w]
            (h,w) = shape(im)
    (X,Y) = meshgrid(arange(w),arange(h))
    x = X.flatten()
    y = Y.flatten()
    wgts = im[y,x]
    sw = sum(wgts)
    if sw == 0:
            return 1
    wgts /= sw
    wpts = array([wgts*x, wgts*y])
    wmean = sum(wpts, 1)
    N = len(x)
    s = array([x,y])
    P = zeros((2,2))
    for i in range(0,N):
            P += wgts[i]*outer(s[:,i],s[:,i])
    P = P - outer(wmean,wmean);

    det = abs(linalg.det(P))
    if (det <= 0):
            return 0.0
    v = linalg.eigvalsh(P)
    v = abs(v)
    r = min(v)/max(v)
    return 100.0*sqrt(r/det)

def image_whiteness(hsv):
    ''' a measure of the whiteness of an HSV image 0 to 1'''
    (width,height) = cv.GetSize(hsv)
    score = 0
    count = 0
    for x in range(width):
            for y in range(height):
                    (h,s,v) = hsv[y,x]
                    if (s < 25 and v > 50):
                            count += 1
    return float(count)/float(width*height)

def raw_hsv_score(hsv):
    '''try to score a HSV image based on hsv'''
    (width,height) = cv.GetSize(hsv)
    score = 0
    blue_count = 0
    red_count = 0
    sum_v = 0

    # keep range of hsv
    h_min = 255
    h_max = 0
    s_min = 255
    s_max = 0
    v_min = 255
    v_max = 0
        
    from numpy import zeros
    scorix = zeros((height,width))
    for x in range(width):
        for y in range(height):
            pix_score = 0
            (h,s,v) = hsv[y,x]
            if h > h_max:
                    h_max = h
            if s > s_max:
                    s_max = s
            if v > v_max:
                    v_max = v
            if h < h_min:
                    h_min = h
            if s < s_min:
                    s_min = s
            if v < v_min:
                    v_min = v
            sum_v += v
            if (h < 22 or (h > 171 and h < 191)) and s > 50 and v < 150:
                pix_score += 3
                blue_count += 1
                #print (x,y),h,s,v,'B'
            elif h > 108 and h < 140 and s > 140 and v > 128:
                pix_score += 1
                red_count += 1
                #print (x,y),h,s,v,'R'
            elif h > 82 and h < 94 and s > 125 and v > 100 and v < 230:
                pix_score += 1
                red_count += 1
                #print (x,y),h,s,v,'Y'
            elif v > 160 and s > 100:
                pix_score += 1
                #print h,s,v,'V'
            elif h>70 and s > 110 and v > 90:
                pix_score += 1
                #print h,s,v,'S'
            score += pix_score
            scorix[y,x] = pix_score

    avg_v = sum_v / (width*height)
    score = 500 * float(score) / (width*height)

    return (score, scorix, blue_count, red_count, avg_v, s_max - s_min, v_max - v_min)

def log_scaling(value, scale):
    # apply a log scaling to a value
    if value <= math.e:
            return scale
    return math.log(value)*scale

def hsv_score(r, hsv, use_compactness=False, use_whiteness=False):
    '''try to score a HSV image based on how "interesting" it is for joe detection'''
    (col_score, scorix, blue_count, red_count, avg_v, s_range, v_range) = raw_hsv_score(hsv)

    r.hsv_score = col_score

    if blue_count < 100 and red_count < 50 and avg_v < 150:
        if blue_count > 1 and red_count > 1:
            col_score *= 2
        if blue_count > 2 and red_count > 2:
            col_score *= 2
        if blue_count > 4 and red_count > 4:
            col_score *= 2

        scorix = (scorix>0).astype(float)

        r.whiteness = image_whiteness(hsv)
    if use_whiteness:
        not_white = 1.0-r.whiteness
        col_score *= not_white
        if r.compactness <= math.e:
                scaled_compactness = 1
        else:
                scaled_compactness = math.log(r.compactness)

        r.col_score = col_score
        if col_score <= math.e:
                scaled_col_score = 1
        else:
                scaled_col_score = math.log(col_score)

        # combine all the scoring systems
        r.score = r.scan_score*(s_range/128.0)*log_scaling(r.compactness,0.2)*log_scaling(col_score,0.3)

def score_region(img, r, filter_type='simple'):
    '''filter a list of regions using HSV values'''
    (x1, y1, x2, y2) = r.tuple()
    if True:
        (w,h) = cuav_util.image_shape(img)
        x = (x1+x2)/2
        y = (y1+y2)/2
        x1 = int(max(x-10,0))
        x2 = int(min(x+10,w))
        y1 = int(max(y-10,0))
        y2 = int(min(y+10,h))
    cv.SetImageROI(img, (x1, y1, x2-x1,y2-y1))
    hsv = cv.CreateImage((x2-x1,y2-y1), 8, 3)
    cv.CvtColor(img, hsv, cv.CV_RGB2HSV)
    cv.ResetImageROI(img)
    if filter_type == 'compactness':
        use_compactness = True
    else:
        use_compactness = False
    hsv_score(r, hsv, use_compactness)

def filter_regions(img, regions, min_score=4, frame_time=None, filter_type='simple'):
    '''filter a list of regions using HSV values'''
    active_regions = []
    img = cv.GetImage(cv.fromarray(img))
    for r in regions:
        if r.scan_score is None:
            score_region(img, r, filter_type=filter_type)
        if r.scan_score >= min_score:
            active_regions.append(r)

    print("  Regions Above Threshold (Active): %i" % len(active_regions))
    return active_regions


def filter_boundary(regions, boundary, pos=None):
    '''filter a list of regions using a search boundary'''
    ret = []
    for r in regions:
        if pos is None:
            continue
        if pos.altitude < 10:
            r.score = 0
            #print pos
            if r.latlon is None or cuav_util.polygon_outside(r.latlon, boundary):
                r.score = 0
            ret.append(r)
    return ret
    
def PruneRegionsBySpectra(regions, bgr):
    neighborhood = 50.0 #REGION_MERGE_PIXEL_DISTANCE
    active_regions = []
    index = 0
    for aregion in regions:
        if aregion.active:
            analyzeRegion(aregion, bgr)

        if aregion.rarity > 100.0 or aregion.fg_rarity > 100.0:
            aregion.active = True
            aregion.priority = REGION_SOMEWHAT_IMPORTANT

        if aregion.active:
            active_regions.append(index)

        index += 1

    print("Prune Regions in Spectral Neighborhood:")
    print("  Neighborhood: %i" % neighborhood)
    print("  Regions Above Neighborhood (Active): %i" % len(active_regions))
    print("  [id]: " + str(sorted(active_regions)))

def PruneRegionsByDupe(regions):
    print("Prune Duplicated Regions:")
    if len(regions) is 0:
        return

    rarity = []
    for region in regions:
        val = int(region.rarity)
        print "region rarity: %i" % val
        rarity.append(val)
    max_rarity = max(rarity)
    like_regions = np.zeros(max_rarity+1)
    print("Max Rarity Detected: %i" % max_rarity)

    print("  Accuracy: %i" % DDUPE_ACCURACY_BY)

    active_regions = []
    index = 0
    for aregion in regions:
        if aregion.active:
            val = int(aregion.rarity) #((aregion.score/DDUPE_ACCURACY_BY)*DDUPE_ACCURACY_BY) / aregion.avg_pixel
            like_regions[val] += 1
            if like_regions[val] > 1:
                aregion.active = False
                aregion.priority = REGION_NOT_IMPORTANT
            else:
                active_regions.append(index)
        index += 1

    if len(regions) > 0 and len(regions) <= MIN_DEDUPE_REGIONS:
        active_regions = []
        index = 0
        for aregion in regions:
            aregion.active = True
            aregion.priority = REGION_SOMEWHAT_IMPORTANT
            active_regions.append(index)
            index += 1
        print("  Reinstated All Regions Below Count: %i" % MIN_DEDUPE_REGIONS)
    elif len([aregion for aregion in regions if aregion.active == True]) == 0:
        active_regions = []
        index = 0
        for aregion in regions:
            if aregion.rarity >= 50 or aregion.fg_rarity >= 50:
                aregion.active = True
                aregion.priority = REGION_SOMEWHAT_IMPORTANT
                active_regions.append(index)
            index += 1
        print("  Reinstated Borderline Regions with Rarity Above: 50")

    print("  Regions Deduped (Active): %i" % len(active_regions))
    print("  [id]: " + str(sorted(active_regions)))

def PruneRegionsByProximity(regions):
    radius = REGION_MERGE_PIXEL_DISTANCE * 10

    active_regions = []
    index = 0
    for aregion in regions:
        if aregion.active:
            if len(active_regions) > 0:
                for i in active_regions:
                    region = regions[i]
                    pregion_centerx = region.x2 #(previous_region.maxx - previous_region.minx)/2
                    pregion_centery = region.y2 #(previous_region.maxy - previous_region.miny)/2
                    aregion_centerx = aregion.x2 #(aregion.maxx - aregion.minx)/2
                    aregion_centery = aregion.y2 #(aregion.maxy - aregion.miny)/2
                    if (aregion_centerx >= max(pregion_centerx-radius,0) and aregion_centerx <= (pregion_centerx+radius)) and \
                       (aregion_centery >= max(pregion_centery-radius,0) and aregion_centery <= (pregion_centery+radius)):
                        aregion.active = False
                        aregion.priority = REGION_NOT_IMPORTANT
                        break

                if aregion.active:
                    active_regions.append(index)
            else:
                active_regions.append(index)
        index += 1

    print("Pruned Clustered Regions:")
    print("  Radius of Separation: %i" % radius)
    print("  Regions Isolated (Active): %i" % len(active_regions))
    print("  [id]: " + str(sorted(active_regions)))

def DrawRegions(regions, bgr):
    mat = cv.GetMat(cv.fromarray(bgr))

    for aregion in regions:
        aregion.draw_rectangle(mat, colour=aregion.priority)

def GetActiveRegions(regions):
    active_regions = []
    for aregion in regions:
        if aregion.active:
            active_regions.append(aregion)

    return active_regions

def analyzeRegion(aregion, bgr):
    mat = cv.GetMat(cv.fromarray(bgr))
    bits = 4
    if not aregion.active:
        return

    (minx,miny,maxx,maxy) = aregion.tuple()
    hist_r = {}
    hist_g = {}
    hist_b = {}
    hist = {}
    for x in range(minx,maxx-1,1):
        for y in range(miny,maxy-1,1):
            (b,g,r) = mat[y,x]
            value = r/(1<<bits)
            count = 0 if not hist_r.has_key(value) else hist_r[value]
            count += 1
            hist_r[value]=count

            value = g/(1<<bits)
            count = 0 if not hist_g.has_key(value) else hist_g[value]
            count += 1
            hist_g[value] = count

            value = b/(1<<bits)
            count = 0 if not hist_b.has_key(value) else hist_b[value]
            count += 1
            hist_b[value] = count

    hist['r'] = hist_r
    hist['g'] = hist_g
    hist['b'] = hist_b

    evaluateHistogram(aregion, hist)

def evaluateHistogram(aregion, hist):
    rlen = len(hist['r'])
    glen = len(hist['g'])
    blen = len(hist['b'])
    if (rlen+blen+glen >= 35) or \
        (rlen == 16 and blen >= 7) or \
        (rlen >= 15 and blen >= 10) or \
        (rlen >= 13 and blen >= 11 and glen >= 11):
        aregion.priority = REGION_IMPORTANT
    elif rlen >= 8 and blen >= 8:
        aregion.priority = REGION_SOMEWHAT_IMPORTANT
    else:
        aregion.active = False
        aregion.priority = REGION_NOT_IMPORTANT

