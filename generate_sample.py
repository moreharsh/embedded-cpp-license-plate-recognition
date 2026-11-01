import cv2, numpy as np
from pathlib import Path
out=Path(__file__).parent/'samples'/'synthetic_plate.png'
h,w=576,1024
im=np.zeros((h,w,3),dtype=np.uint8)
for y in range(h):
    im[y,:,:]=(max(25,int(95-y*.12)),max(25,int(100-y*.11)),max(25,int(105-y*.10)))
# simplified vehicle silhouette: a dark gray body with white license plate
cv2.rectangle(im,(130,185),(890,510),(45,56,66),-1)
cv2.rectangle(im,(195,148),(830,268),(30,35,42),-1)
cv2.rectangle(im,(210,172),(812,260),(60,75,87),-1)
cv2.rectangle(im,(178,312),(302,359),(36,44,185),-1)
cv2.rectangle(im,(719,312),(844,359),(36,44,185),-1)
cv2.rectangle(im,(185,486),(320,529),(26,28,30),-1)
cv2.rectangle(im,(716,486),(846,529),(26,28,30),-1)
# plate region
x1,y1,x2,y2=360,374,670,459
cv2.rectangle(im,(x1,y1),(x2,y2),(8,8,8),-1)
cv2.rectangle(im,(x1+5,y1+5),(x2-5,y2-5),(245,245,245),-1)
label='TEST123'
font=cv2.FONT_HERSHEY_SIMPLEX
scale=1.8
thickness=4
(tw,th),base=cv2.getTextSize(label,font,scale,thickness)
cv2.putText(im,label,((x1+x2-tw)//2,(y1+y2+th)//2-1),font,scale,(12,12,12),thickness,cv2.LINE_AA)
cv2.imwrite(str(out),im)
print(str(out),im.shape,tw)
