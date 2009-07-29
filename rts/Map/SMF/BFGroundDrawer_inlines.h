#ifndef __BF_GROUND_DRAWER_INLINES_H__
#define __BF_GROUND_DRAWER_INLINES_H__

#include "Game/Camera.h"
#include "Map/ReadMap.h"


/* inlines originally in BFGroundDrawer.cpp moved here, so they can be used
   from other files too (by including this file after BFGroundDrawer.h) */

inline void CBFGroundDrawer::AdvDraw(int bty) {

	float cx2 = cam2->pos.x ;
	float cz2 = cam2->pos.z ;
	float cy2 = cam2->pos.y ;
	//CVertexArray *ma = GetVertexArray();
	//cam2->
	bool framchanged=false;
	if (prevcam[0]!=cam2->pos.x) framchanged=true;
	else if (prevcam[1]!=cam2->pos.y) framchanged=true;
	else if (prevcam[2]!=cam2->pos.z) framchanged=true;
	else if (prevcam[3]!=cam2->forward.x) framchanged=true;
	else if (prevcam[4]!=cam2->forward.y) framchanged=true;
	else if (prevcam[5]!=cam2->forward.z) framchanged=true;



	//ma->Initialize();
	//ma->EnlargeArrays(1000000,1000000);
	if (framchanged){
		landscape.Reset();
		landscape.Tessellate(cx2,cy2,cz2,viewRadius);
		prevcam[0]=cam2->pos.x;
		prevcam[1]=cam2->pos.y;
		prevcam[2]=cam2->pos.z;
		prevcam[3]=cam2->forward.x;
		prevcam[4]=cam2->forward.y;
		prevcam[5]=cam2->forward.z;

	}

	int patchesdrawn=0;
	for(int i=0;i< (gs->mapx/PATCH_SIZE)*(gs->mapy/PATCH_SIZE);i++){
		if (landscape.m_Patches[i].isVisibile()==1) patchesdrawn++;
	}
	patchesdrawn++;

	landscape.Render(this);
	//SetupBigSquare(btx,bty);
	//DrawGroundVertexArrayQ(ma);

}


inline void CBFGroundDrawer::DrawWaterPlane(bool drawWaterReflection) {
	if (!drawWaterReflection) {
		const bool camOutOfMap = !camera->pos.IsInBounds();
		glCallList(camOutOfMap ? waterPlaneCamOutDispList : waterPlaneCamInDispList);
	}
}


inline void CBFGroundDrawer::DrawVertexAQ(CVertexArray *ma, int x, int y)
{
	float height = heightData[y * heightDataX + x];
	if (waterDrawn && height < 0) {
		height *= 2;
	}

	ma->AddVertexQ0(x * SQUARE_SIZE, height, y * SQUARE_SIZE);
}


inline void CBFGroundDrawer::DrawVertexAQ(CVertexArray *ma, int x, int y, float height)
{
	if (waterDrawn && height < 0) {
		height *= 2;
	}
	ma->AddVertexQ0(x * SQUARE_SIZE, height, y * SQUARE_SIZE);
}


inline void CBFGroundDrawer::EndStripQ(CVertexArray *ma)
{
	ma->EndStripQ();
}


inline bool CBFGroundDrawer::BigTexSquareRowVisible(int bty) {
	const int minx =             0;
	const int maxx =      mapWidth;
	const int minz = bty * bigTexH;
	const int maxz = minz + bigTexH;
	const float miny = readmap->currMinHeight;
	const float maxy = fabs(cam2->pos.y); // ??

	const float3 mins(minx, miny, minz);
	const float3 maxs(maxx, maxy, maxz);

	return (cam2->InView(mins, maxs));
}


inline void CBFGroundDrawer::DrawGroundVertexArrayQ(CVertexArray * &ma)
{
	ma->DrawArray0(GL_TRIANGLE_STRIP);
	ma = GetVertexArray();
}


inline void CBFGroundDrawer::FindRange(int &xs, int &xe, const std::vector<fline> &left, const std::vector<fline> &right, int y, int lod) {
	int xt0, xt1;
	std::vector<fline>::const_iterator fli;

	for (fli = left.begin(); fli != left.end(); fli++) {
		float xtf = fli->base + fli->dir * y;
		xt0 = (int)xtf;
		xt1 = (int)(xtf + fli->dir * lod);

		if (xt0 > xt1)
			xt0 = xt1;

		xt0 = xt0 / lod * lod - lod;

		if (xt0 > xs)
			xs = xt0;
	}
	for (fli = right.begin(); fli != right.end(); fli++) {
		float xtf = fli->base + fli->dir * y;
		xt0 = (int)xtf;
		xt1 = (int)(xtf + fli->dir * lod);

		if (xt0 < xt1)
			xt0 = xt1;

		xt0 = xt0 / lod * lod + lod;

		if (xt0 < xe)
			xe = xt0;
	}
}


#define CLAMP(i) std::max(0, std::min((i), maxIdx))

inline void CBFGroundDrawer::DoDrawGroundRow(int bty) {
	if (!BigTexSquareRowVisible(bty)) {
		//! skip this entire row of squares if we can't see it
		//return;
	}

	CVertexArray *ma = GetVertexArray();

	bool inStrip = false;
	float x0, x1;
	int x,y;
	int sx = 0;
	int ex = numBigTexX;
	std::vector<fline>::iterator fli;

	//! only process the necessary big squares in the x direction
	/*int bigSquareSizeY = bty * bigSquareSize;
	for (fli = left.begin(); fli != left.end(); fli++) {
		x0 = fli->base + fli->dir * bigSquareSizeY;
		x1 = x0 + fli->dir * bigSquareSize;

		if (x0 > x1)
			x0 = x1;

		x0 /= bigSquareSize;

		if (x0 > sx)
			sx = (int) x0;
	}
	for (fli = right.begin(); fli != right.end(); fli++) {
		x0 = fli->base + fli->dir * bigSquareSizeY + bigSquareSize;
		x1 = x0 + fli->dir * bigSquareSize;

		if (x0 < x1)
			x0 = x1;

		x0 /= bigSquareSize;

		if (x0 < ex)
			ex = (int) x0;
	}
*/
	float cx2 = cam2->pos.x / (SQUARE_SIZE*bigSquareSize);
	float cz2 = cam2->pos.z / (SQUARE_SIZE*bigSquareSize);
	float cy2 = cam2->pos.y / (SQUARE_SIZE*bigSquareSize);

/*	for (int btx = 0; btx < ex; ++btx) {
		ma->Initialize();
		float dstm= sqrt((bty-cz2)*(bty-cz2)+(btx-cx2)*(btx-cx2))+cy2;
		dstm=dstm*dstm;
		for(int i=0;i<(map->height+1)*w;i++){ altheight[i]=32768}
		int nloop=tree[ex*bty+btx]->CountSub(max((int)(viewRadius*dstm),viewRadius,altheight,heightdata));
		ma->EnlargeArrays(4*nloop,nloop);
		tree[ex*bty+btx]->Draw(ma,altheight,max((int)(viewRadius*dstm),viewRadius));

		for (inti=0;i<ps;i++){
			if(shit[i]->diff<viewradius || shit[i]->leaf){
				if (altheight[w*shit[i]->y1+shit[i]->x1]==32768){

				}
			}
		}

		SetupBigSquare(btx,bty);
		DrawGroundVertexArrayQ(ma);
	}*/
}

#endif
