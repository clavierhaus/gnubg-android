#!/usr/bin/env python3
"""Plus-edition launcher icons: the S2 mark with ALL letters in brand orange
(the free edition sets only the C in orange). Dev-visible edition tell."""
from PIL import Image, ImageDraw, ImageFont
BLACK=(0,0,0,255); ORANGE=(245,166,35,255); ORANGE50=(245,166,35,128)
FONT='gnubg-app/app/src/main/res/font/dejavu_serif_bold.ttf'
SS=4
def paint(s, transparent_bg):
    im=Image.new("RGBA",(s,s),(0,0,0,0) if transparent_bg else BLACK)
    layer=Image.new("RGBA",(s,s),(0,0,0,0)); ld=ImageDraw.Draw(layer)
    r=s*0.40; cx=cy=s/2
    ld.ellipse([cx-r,cy-r,cx+r,cy+r],fill=ORANGE50)
    lw=max(2,int(s*0.016)); rr=r*0.70
    ld.ellipse([cx-rr,cy-rr,cx+rr,cy+rr],outline=(0,0,0,128),width=lw)
    im=Image.alpha_composite(im,layer); d=ImageDraw.Draw(im)
    f=ImageFont.truetype(FONT,int(s*0.40))
    glyphs=[]; total=0
    for ch in "CBG":
        bb=d.textbbox((0,0),ch,font=f); w=bb[2]-bb[0]; h=bb[3]-bb[1]
        glyphs.append((ch,w,h,bb)); total+=w
    track=s*0.015; total+=track*2; x=(s-total)/2
    for ch,w,h,bb in glyphs:
        d.text((x-bb[0],(s-h)/2-bb[1]),ch,font=f,fill=ORANGE)
        x+=w+track
    return im
s288=288*SS
motif=paint(s288,True)
canvas=Image.new("RGBA",(432*SS,432*SS),(0,0,0,0))
canvas.paste(motif,((432*SS-s288)//2,(432*SS-s288)//2),motif)
canvas.resize((432,432),Image.LANCZOS).save('gnubg-app/app/src/main/res/drawable/ic_launcher_foreground.png')
for dpi,px in [('mdpi',48),('hdpi',72),('xhdpi',96),('xxhdpi',144),('xxxhdpi',192)]:
    im=paint(px*SS,False).resize((px,px),Image.LANCZOS)
    im.save(f'gnubg-app/app/src/main/res/mipmap-{dpi}/ic_launcher.png')
    im.save(f'gnubg-app/app/src/main/res/mipmap-{dpi}/ic_launcher_round.png')
print("Plus icons written (all-orange letters)")
