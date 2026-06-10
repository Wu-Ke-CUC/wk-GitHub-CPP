# Mesh Subdivisions

This project implements three subdivision algorithms:

- Loop
- Catmull-Clark
- Doo-Sabin

## Open In Visual Studio 2019

- Open `MeshSubdivisionViewer.sln`.
- Select `Debug` or `Release`.
- Select `Win32` or `x64`.
- Build and run `MeshSubdivisionViewer`.

The executable searches upward from its output directory until it finds the project's `models`
folder, so it can run from the standard Visual Studio output layout.

## Controls

- `Left` / `Right`: switch between subdivision scenes
- `Up` / `Down`: switch subdivision level from `0` to `3`
- Drag with the left mouse button: rotate the mesh
- Mouse wheel: zoom in or out
- `R`: reset the camera

## Scenes Included

- Loop / tetrahedron
- Loop / cube_tri
- Catmull-Clark / tetrahedron
- Catmull-Clark / cube_quad
- Doo-Sabin / tetrahedron
- Doo-Sabin / cube_quad

Level `0` shows the source mesh. Levels `1` to `3` show the progressively subdivided result.
