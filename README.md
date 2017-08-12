# CDLOD
CDLOD (2010) paper and collateral data and source code

http://www.tandfonline.com/doi/abs/10.1080/2151237X.2009.10129287


Abstract

"This paper presents a technique for GPU-based rendering of heightmap terrains, which is a refinement of several existing methods with some new ideas. It is similar to the terrain clipmap approaches [Tanner et al. 98, Losasso 04], as it draws the terrain directly from the source heightmap data. However, instead of using a set of regular nested grids, it is structured around a quadtree of regular grids, more similar to [Ulrich 02], which provides it with better level-of-detail distribution. The algorithm's main improvement over previous techniques is that the LOD function is the same across the whole rendered mesh and is based on the precise three-dimensional distance between the observer and the terrain. To accomplish this, a novel technique for handling transition between LOD levels is used, which gives smooth and accurate results. For these reasons the system is more predictable and reliable, with better screen-triangle distribution, cleaner transitions between levels, and no need for stitching meshes. This also simplifies integration with other LOD systems that are common in games and simulation applications. With regard to the performance, it remains favourable compared to similar GPU-based approaches and works on all graphics hardware supporting Shader Model 3.0 and above. Demo and complete source code is available online under a free software license."
