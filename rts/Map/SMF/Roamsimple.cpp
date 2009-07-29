//
// ROAM Simplistic Implementation
//Added to Spring by Peter Sarkozy (mysterme AT gmail DOT com)
// Billion thanks to Bryan Turner (Jan, 2000)
//                    brturn@bellsouth.net
//
// Based on the Tread Marks engine by Longbow Digital Arts
//                               (www.LongbowDigitalArts.com)
// Much help and hints provided by Seumas McNally, LDA.
//
//#include <windows.h>
//#include <math.h>
//#include <gl/gl.h>		// OpenGL


#include "Rendering/GL/VertexArray.h"
#include "BFGroundDrawer.h"
#include "BFGroundDrawer_inlines.h"
#include "Landscape.h"
#include "Game/Camera.h"
//#include "FastMath.h"

// -------------------------------------------------------------------------------------------------
//	PATCH CLASS
// -------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------
// Split a single Triangle and link it into the mesh.
// Will correctly force-split diamonds.
//
void Patch::Split(TriTreeNode *tri)
{
	// We are already split, no need to do it again.
	if (tri->LeftChild)
		return;

	// If this triangle is not in a proper diamond, force split our base neighbor
	if ( tri->BaseNeighbor && (tri->BaseNeighbor->BaseNeighbor != tri) )
		Split(tri->BaseNeighbor);

	// Create children and link into mesh
	tri->LeftChild  = Landscape::AllocateTri();
	tri->RightChild = Landscape::AllocateTri();

	// If creation failed, just exit.
	if ( !tri->LeftChild )
		return;

	// Fill in the information we can get from the parent (neighbor pointers)
	tri->LeftChild->BaseNeighbor  = tri->LeftNeighbor;
	tri->LeftChild->LeftNeighbor  = tri->RightChild;

	tri->RightChild->BaseNeighbor  = tri->RightNeighbor;
	tri->RightChild->RightNeighbor = tri->LeftChild;

	// Link our Left Neighbor to the new children
	if (tri->LeftNeighbor != NULL)
	{
		if (tri->LeftNeighbor->BaseNeighbor == tri)
			tri->LeftNeighbor->BaseNeighbor = tri->LeftChild;
		else if (tri->LeftNeighbor->LeftNeighbor == tri)
			tri->LeftNeighbor->LeftNeighbor = tri->LeftChild;
		else if (tri->LeftNeighbor->RightNeighbor == tri)
			tri->LeftNeighbor->RightNeighbor = tri->LeftChild;
		else
			;// Illegal Left Neighbor!
	}

	// Link our Right Neighbor to the new children
	if (tri->RightNeighbor != NULL)
	{
		if (tri->RightNeighbor->BaseNeighbor == tri)
			tri->RightNeighbor->BaseNeighbor = tri->RightChild;
		else if (tri->RightNeighbor->RightNeighbor == tri)
			tri->RightNeighbor->RightNeighbor = tri->RightChild;
		else if (tri->RightNeighbor->LeftNeighbor == tri)
			tri->RightNeighbor->LeftNeighbor = tri->RightChild;
		else
			;// Illegal Right Neighbor!
	}

	// Link our Base Neighbor to the new children
	if (tri->BaseNeighbor != NULL)
	{
		if ( tri->BaseNeighbor->LeftChild )
		{
			tri->BaseNeighbor->LeftChild->RightNeighbor = tri->RightChild;
			tri->BaseNeighbor->RightChild->LeftNeighbor = tri->LeftChild;
			tri->LeftChild->RightNeighbor = tri->BaseNeighbor->RightChild;
			tri->RightChild->LeftNeighbor = tri->BaseNeighbor->LeftChild;
		}
		else
			Split( tri->BaseNeighbor);  // Base Neighbor (in a diamond with us) was not split yet, so do that now.
	}
	else
	{
		// An edge triangle, trivial case.
		tri->LeftChild->RightNeighbor = NULL;
		tri->RightChild->LeftNeighbor = NULL;
	}
}

// ---------------------------------------------------------------------
// Tessellate a Patch.
// Will continue to split until the variance metric is met.
//
void Patch::RecursTessellate( TriTreeNode *tri,
							 int leftX,  int leftY,
							 int rightX, int rightY,
							 int apexX,  int apexY,
							 int node )
{
	float TriVariance;
	int centerX = (leftX + rightX)>>1; // Compute X coordinate of center of Hypotenuse
	int centerY = (leftY + rightY)>>1; // Compute Y coord...

	if ( node < (1<<VARIANCE_DEPTH) )
	{
		TriVariance = ((float)m_CurrentVariance[node] *PATCH_SIZE*8)/distfromcam;	// Take both distance and variance into consideration
	}

	if ( (node >= (1<<VARIANCE_DEPTH)) ||	// IF we do not have variance info for this node, then we must have gotten here by splitting, so continue down to the lowest level.
		 (TriVariance > gFrameVariance))	// OR if we are not below the variance tree, test for variance.
	{
		Split(tri);														// Split this triangle.

		if (tri->LeftChild &&											// If this triangle was split, try to split it's children as well.
			((abs(leftX - rightX) >= 3) || (abs(leftY - rightY) >= 3)))	// Tessellate all the way down to one vertex per height field entry
		{
			RecursTessellate( tri->LeftChild,   apexX,  apexY, leftX, leftY, centerX, centerY,    node<<1  );
			RecursTessellate( tri->RightChild, rightX, rightY, apexX, apexY, centerX, centerY, 1+(node<<1) );
		}
	}
}

// ---------------------------------------------------------------------
// Render the tree.  Simple no-fan method.
//
void Patch::RecursRender( TriTreeNode *tri, int leftX, int leftY, int rightX, int rightY, int apexX, int apexY, CVertexArray *ma,CBFGroundDrawer * parent,int n)
{
	if ( tri->LeftChild )					// All non-leaf nodes have both children, so just check for one
	{
		int centerX = (leftX + rightX)>>1;	// Compute X coordinate of center of Hypotenuse
		int centerY = (leftY + rightY)>>1;	// Compute Y coord...

		RecursRender( tri->LeftChild,  apexX,   apexY, leftX, leftY, centerX, centerY ,ma,parent,n);
		RecursRender( tri->RightChild, rightX, rightY, apexX, apexY, centerX, centerY ,ma ,parent,n);
	}
	else									// A leaf node!  Output a triangle to be rendered.
	{
		// Actual number of rendered triangles...
		//gNumTrisRendered++;
		parent->DrawVertexAQ(ma,leftX+m_WorldX,leftY+m_WorldY);
		parent->DrawVertexAQ(ma,rightX+m_WorldX,rightY+m_WorldY);
		parent->DrawVertexAQ(ma,apexX+m_WorldX,apexY+m_WorldY);
		parent->EndStripQ(ma);

	}
}

// ---------------------------------------------------------------------
// Computes Variance over the entire tree.  Does not examine node relationships.
//
unsigned char Patch::RecursComputeVariance( int leftX,  int leftY,  unsigned char leftZ,
										    int rightX, int rightY, unsigned char rightZ,
											int apexX,  int apexY,  unsigned char apexZ,
											int node)
{
	//        /|\
	//      /  |  \
	//    /    |    \
	//  /      |      \
	//  ~~~~~~~*~~~~~~~  <-- Compute the X and Y coordinates of '*'
	//
	int centerX = (leftX + rightX) >>1;		// Compute X coordinate of center of Hypotenuse
	int centerY = (leftY + rightY) >>1;		// Compute Y coord...
	unsigned char myVariance;

	// Get the height value at the middle of the Hypotenuse
	unsigned char centerZ  = m_HeightMap[(centerY * mapx) + centerX];
	// Variance of this triangle is the actual height at it's hypotenuse midpoint minus the interpolated height.
	// Use values passed on the stack instead of re-accessing the Height Field.
	myVariance = abs((int)centerZ - (((int)leftZ + (int)rightZ)>>1));

	// Since we're after speed and not perfect representations,
	//    only calculate variance down to an 8x8 block
	if ( (abs(leftX - rightX) >= 8) ||
		 (abs(leftY - rightY) >= 8) )
	{
		// Final Variance for this node is the max of it's own variance and that of it's children.
		myVariance = MAX( myVariance, RecursComputeVariance( apexX,   apexY,  apexZ, leftX, leftY, leftZ, centerX, centerY, centerZ,    node<<1 ) );
		myVariance = MAX( myVariance, RecursComputeVariance( rightX, rightY, rightZ, apexX, apexY, apexZ, centerX, centerY, centerZ, 1+(node<<1)) );
	}

	// Store the final variance for this node.  Note Variance is never zero.
	if (node < (1<<VARIANCE_DEPTH))
		m_CurrentVariance[node] = 1 + myVariance;

	return myVariance;
}

// -------------------------------------------------------------------------------------------------
//	PATCH CLASS
// -------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------
// Initialize a patch.
//
void Patch::Init( int heightX, int heightY, int worldX, int worldY, unsigned char *hMap , int mx,float maxH,float minH)
{
	// Clear all the relationships
	maxh=maxH;
	minh=minH;
	mapx=mx;
	m_BaseLeft.RightNeighbor = m_BaseLeft.LeftNeighbor = m_BaseRight.RightNeighbor = m_BaseRight.LeftNeighbor =
	m_BaseLeft.LeftChild = m_BaseLeft.RightChild = m_BaseRight.LeftChild = m_BaseLeft.LeftChild = NULL;

	// Attach the two m_Base triangles together
	m_BaseLeft.BaseNeighbor = &m_BaseRight;
	m_BaseRight.BaseNeighbor = &m_BaseLeft;

	// Store Patch offsets for the world and heightmap.
	m_WorldX = worldX;
	m_WorldY = worldY;

	// Store pointer to first byte of the height data for this patch.
	m_HeightMap = &hMap[heightY * mapx + heightX];

	// Initialize flags
	m_VarianceDirty = 1;
	m_isVisible = 0;
}

// ---------------------------------------------------------------------
// Reset the patch.
//
void Patch::Reset()
{
	// Assume patch is not visible.
	m_isVisible = 0;

	// Reset the important relationships
	m_BaseLeft.LeftChild = m_BaseLeft.RightChild = m_BaseRight.LeftChild = m_BaseLeft.LeftChild = NULL;

	// Attach the two m_Base triangles together
	m_BaseLeft.BaseNeighbor = &m_BaseRight;
	m_BaseRight.BaseNeighbor = &m_BaseLeft;

	// Clear the other relationships.
	m_BaseLeft.RightNeighbor = m_BaseLeft.LeftNeighbor = m_BaseRight.RightNeighbor = m_BaseRight.LeftNeighbor = NULL;
}

// ---------------------------------------------------------------------
// Compute the variance tree for each of the Binary Triangles in this patch.
//
void Patch::ComputeVariance()
{
	// Compute variance on each of the base triangles...

	m_CurrentVariance = m_VarianceLeft;
	RecursComputeVariance(	0,          PATCH_SIZE, m_HeightMap[PATCH_SIZE * mapx],
							PATCH_SIZE, 0,          m_HeightMap[PATCH_SIZE],
							0,          0,          m_HeightMap[0],
							1);

	m_CurrentVariance = m_VarianceRight;
	RecursComputeVariance(	PATCH_SIZE, 0,          m_HeightMap[ PATCH_SIZE],
							0,          PATCH_SIZE, m_HeightMap[ PATCH_SIZE * mapx],
							PATCH_SIZE, PATCH_SIZE, m_HeightMap[(PATCH_SIZE * mapx) + PATCH_SIZE],
							1);

	// Clear the dirty flag for this patch
	m_VarianceDirty = 0;
}


// ---------------------------------------------------------------------
// Set patch's visibility flag.
//
void Patch::SetVisibility( int eyeX, int eyeY, int leftX, int leftY, int rightX, int rightY )

{
	float3 patchpos1,patchpos2;
	patchpos1.x=(m_WorldX+PATCH_SIZE/2)*8;
	patchpos1.z=(m_WorldY+PATCH_SIZE/2)*8;
	patchpos1.y=(maxh+minh)/2;
	if(cam2->InView(patchpos1,768)) m_isVisible=1;
	else m_isVisible=0;
	//m_isVisible = 1;
}

// ---------------------------------------------------------------------
// Create an approximate mesh.
//
void Patch::Tessellate(float cx,float cy, float cz, int viewradius)
{
	float myx=(m_WorldX+PATCH_SIZE/2)*8;
	float myz=(m_WorldY+PATCH_SIZE/2)*8;

	distfromcam= (abs(cx-myx) +cy +abs(cz-myz))*((float)40/viewradius);
	// Split each of the base triangles
	m_CurrentVariance = m_VarianceLeft;
	RecursTessellate (	&m_BaseLeft,
						m_WorldX,				m_WorldY+PATCH_SIZE,
						m_WorldX+PATCH_SIZE,	m_WorldY,
						m_WorldX,				m_WorldY,
						1 );

	m_CurrentVariance = m_VarianceRight;
	RecursTessellate(	&m_BaseRight,
						m_WorldX+PATCH_SIZE,	m_WorldY,
						m_WorldX,				m_WorldY+PATCH_SIZE,
						m_WorldX+PATCH_SIZE,	m_WorldY+PATCH_SIZE,
						1 );
}

// ---------------------------------------------------------------------
// Render the mesh.
//
void Patch::Render(CVertexArray *ma,CBFGroundDrawer * parent, int n)
{

		RecursRender (	&m_BaseLeft,
			0,				PATCH_SIZE,
			PATCH_SIZE,		0,
			0,				0,
			ma,parent,n);

		RecursRender(	&m_BaseRight,
			PATCH_SIZE,		0,
			0,				PATCH_SIZE,
			PATCH_SIZE,		PATCH_SIZE,
			ma,parent,n);

}

// -------------------------------------------------------------------------------------------------
//	LANDSCAPE CLASS
// -------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------
// Definition of the static member variables
//
int         Landscape::m_NextTriNode;
TriTreeNode Landscape::m_TriPool[POOL_SIZE];

// ---------------------------------------------------------------------
// Allocate a TriTreeNode from the pool.
//
TriTreeNode *Landscape::AllocateTri()
{
	TriTreeNode *pTri;

	// IF we've run out of TriTreeNodes, just return NULL (this is handled gracefully)
	if ( m_NextTriNode >= POOL_SIZE )
		return NULL;

	pTri = &(m_TriPool[m_NextTriNode++]);
	pTri->LeftChild = pTri->RightChild = NULL;

	return pTri;
}

// ---------------------------------------------------------------------
// Initialize all patches
//
void Landscape::Init(unsigned char *hMap, int bx, int by,float * heightData)//513 513
{
	Patch *patch;
	int X, Y;
	this->heightData=heightData;

	// Store the Height Field array
	m_HeightMap = hMap;
	h=by;
	w=bx;
	m_Patches=new Patch[(bx/PATCH_SIZE)*(by/PATCH_SIZE)];
	minhpatch=new float[(bx/PATCH_SIZE)*(by/PATCH_SIZE)];
	maxhpatch=new float[(bx/PATCH_SIZE)*(by/PATCH_SIZE)];

	for (int i=0; i<(bx/PATCH_SIZE)*(by/PATCH_SIZE);i++){
		maxhpatch[i]=-10000;
		minhpatch[i]=10000;
	}

	// Initialize all terrain patches
	for ( Y=0; Y < by/PATCH_SIZE; Y++)
		for ( X=0; X < bx/PATCH_SIZE; X++ )
		{
			patch = &(m_Patches[Y*(bx/PATCH_SIZE)+X]);
			for(int i=0;i<PATCH_SIZE;i++){
				for(int j=0;j<PATCH_SIZE;j++){
					maxhpatch[Y*bx/PATCH_SIZE+X]=MAX(maxhpatch[Y*bx/PATCH_SIZE+X], heightData[X*PATCH_SIZE+i+(Y*PATCH_SIZE+j)*(bx+1)]);
					minhpatch[Y*bx/PATCH_SIZE+X]=MIN(minhpatch[Y*bx/PATCH_SIZE+X], heightData[X*PATCH_SIZE+i+(Y*PATCH_SIZE+j)*(bx+1)]);
				}
			}
			patch->Init( X*PATCH_SIZE, Y*PATCH_SIZE, X*PATCH_SIZE, Y*PATCH_SIZE, hMap ,bx,maxhpatch[Y*bx/PATCH_SIZE+X],minhpatch[Y*bx/PATCH_SIZE+X]);
			patch->ComputeVariance();
		}
}

// ---------------------------------------------------------------------
// Reset all patches, recompute variance if needed
//
void Landscape::Reset()
{
	//
	// Perform simple visibility culling on entire patches.
	//   - Define a triangle set back from the camera by one patch size, following
	//     the angle of the frustum.
	//   - A patch is visible if it's center point is included in the angle: Left,Eye,Right
	//   - This visibility test is only accurate if the camera cannot look up or down significantly.
	//
	const float PI_DIV_180 = M_PI / 180.0f;
	const float FOV_DIV_2 = gFovX/2;

	int eyeX =1000;
	int eyeY =1000;
	int leftX  =1002;
	int leftY  = 1200;

	int rightX = 1300;
	int rightY = 1300;
	int X, Y;
	Patch *patch;

	// Set the next free triangle pointer back to the beginning
	SetNextTriNode(0);

	// Reset rendered triangle count.
	gNumTrisRendered = 0;

	// Go through the patches performing resets, compute variances, and linking.
	for ( Y=0; Y < h/PATCH_SIZE; Y++ )
		for ( X=0; X < w/PATCH_SIZE; X++)
		{
			patch = &(m_Patches[Y*(w/PATCH_SIZE)+X]);

			// Reset the patch
			patch->Reset();
			patch->SetVisibility( eyeX, eyeY, leftX, leftY, rightX, rightY );

			// Check to see if this patch has been deformed since last frame.
			// If so, recompute the varience tree for it.
			if ( patch->isDirty() )
				patch->ComputeVariance();

			if ( patch->isVisibile() )
			{
				// Link all the patches together.
				if ( X > 0 )
					patch->GetBaseLeft()->LeftNeighbor = m_Patches[Y*(w/PATCH_SIZE)+X-1].GetBaseRight();
				else
					patch->GetBaseLeft()->LeftNeighbor = NULL;		// Link to bordering Landscape here..

				if ( X < ((w/PATCH_SIZE)-1) )
					patch->GetBaseRight()->LeftNeighbor = m_Patches[Y*(w/PATCH_SIZE)+X+1].GetBaseLeft();
				else
					patch->GetBaseRight()->LeftNeighbor = NULL;		// Link to bordering Landscape here..

				if ( Y > 0 )
					patch->GetBaseLeft()->RightNeighbor = m_Patches[(Y-1)*(w/PATCH_SIZE)+X].GetBaseRight();
				else
					patch->GetBaseLeft()->RightNeighbor = NULL;		// Link to bordering Landscape here..

				if ( Y < ((h/PATCH_SIZE)-1) )
					patch->GetBaseRight()->RightNeighbor = m_Patches[(Y+1)*(w/PATCH_SIZE)+X].GetBaseLeft();
				else
					patch->GetBaseRight()->RightNeighbor = NULL;	// Link to bordering Landscape here..
			}
		}

}

// ---------------------------------------------------------------------
// Create an approximate mesh of the landscape.
//
void Landscape::Tessellate(float cx,float cy, float cz,int viewradius)
{
	// Perform Tessellation
	int nCount;
	Patch *patch = &(m_Patches[0]);


	for (nCount=0; nCount < (w/PATCH_SIZE)*(h/PATCH_SIZE); nCount++, patch++ )
	{


		if (patch->isVisibile())
			patch->Tessellate(cx,cy,cz,viewradius );
	}
}

// ---------------------------------------------------------------------
// Render each patch of the landscape & adjust the frame variance.
//
void Landscape::Render(CBFGroundDrawer * parent)
{
	int nCount;
	Patch *patch = &(m_Patches[0]);

	// Scale the terrain by the terrain scale specified at compile time.


	for (nCount=0; nCount < (w/PATCH_SIZE)*(h/PATCH_SIZE); nCount++, patch++ )
	{


		if (patch->isVisibile()){
			CVertexArray *ma = GetVertexArray();
			ma->Initialize();
			ma->EnlargeArrays(100010,100010);
			patch->Render(ma,parent,nCount);
			parent->SetupBigSquare(nCount%(w/PATCH_SIZE),nCount/(w/PATCH_SIZE));
			parent->DrawGroundVertexArrayQ(ma);
		}

	}

	// Check to see if we got close to the desired number of triangles.
	// Adjust the frame variance to a better value.
	if ( GetNextTriNode() != gDesiredTris )
		gFrameVariance += ((float)GetNextTriNode() - (float)gDesiredTris) / (float)gDesiredTris;

	// Bounds checking.
	//if ( gFrameVariance < 0.5 )
		gFrameVariance = 1.25;
}




