#!/usr/bin/python3
import numpy as np
import random

# see https://sighack.com/post/poisson-disk-sampling-bridsons-algorithm

N = 3
dimensions = np.array([ 1.0, 1.0, 1.0 ])

def sphereSample(rMin, rMax):
    sample = [0, 0, 0]
    while np.linalg.norm(sample) < 0.01 or np.linalg.norm(sample) > 1.0:
        sample[0] = random.uniform(-1.0, 1.0)
        sample[1] = random.uniform(-1.0, 1.0)
        sample[2] = random.uniform(-1.0, 1.0)
    sample = sample / np.linalg.norm(sample)
    # this does not uniformly draw from all points, since there should be more points near rMax
    # but it will do
    len = random.uniform(rMin, rMax)
    return sample * len

def toIndices(point, cellSize):
    return (point / cellSize).astype(int)

def valid(grid, points, cellSize, cellCount, p, radius):
    if (p < 0).any() or (p >= dimensions).any():
        return False

    indices = toIndices(p, cellSize)
    i0 = max(indices[0] - 2, 0)
    i1 = min(indices[0] + 2, cellCount[0] - 1)
    j0 = max(indices[1] - 2, 0)
    j1 = min(indices[1] + 2, cellCount[1] - 1)
    k0 = max(indices[2] - 2, 0)
    k1 = min(indices[2] + 2, cellCount[2] - 1)

    for i in range(i0, i1 + 1):
        for j in range(j0, j1 + 1):
            for k in range(k0, k1 + 1):
                if grid[i, j, k] >= 0:
                    if np.linalg.norm(points[grid[i, j, k]] - p) < radius:
                        return False

    return True

def poissonDiskSampling(radius, k):
    points = []
    active = []
    p0 = np.array([ random.uniform(0, dimensions[0]), random.uniform(0, dimensions[1]), random.uniform(0, dimensions[2]) ])
    cellSize = radius/np.sqrt(N)
    cellCount = (np.ceil(dimensions/cellSize) + 1).astype(int)
    grid = np.zeros(cellCount, dtype=int) - 1
    print(f"Total cell count: {cellCount[0] * cellCount[1] * cellCount[2]}")

    grid[tuple(toIndices(p0, cellSize))] = len(points)
    points.append(p0)
    active.append(p0)

    while len(active) > 0:
        print(f"Active: {len(active)}")
        i = random.randint(0,len(active)-1)
        p = active[i]
        
        found = False
        for attempt in range(0, k):
            pNew = p + sphereSample(radius, 2 * radius)      
            if not valid(grid, points, cellSize, cellCount, pNew, radius):
                continue
            
            indices = tuple(toIndices(pNew, cellSize))
            if grid[indices] != -1:
                raise Exception("[BUG] Grid cell was already occupied! Should not have been valid.")
            print(f"{indices}: {pNew}")
            grid[indices] = len(points)
            points.append(pNew)
            active.append(pNew)
            found = True

            break
        if not found:
            active.pop(i)
        
        for i, x in enumerate(points):
            for j, y in enumerate(points):
                if i == j:
                    continue
                if np.linalg.norm(x - y) < radius:
                    raise Exception(f"[BUG] Invalid samples {i}: {x}, {j}: {y} generated with radius {radius}!")
    return points

def checkedPoissonDiskSampling(radius, k):
    samples = poissonDiskSampling(radius, k)
    print("Checking samples...")
    for i, x in enumerate(samples):
        if (x < 0).any() or (x >= dimensions).any():
            raise Exception("[BUG] Invalid samples generated!")
        for j, y in enumerate(samples):
            if i == j:
                continue
            if np.linalg.norm(x - y) < radius:
                raise Exception(f"[BUG] Invalid samples {x}, {y} generated with radius {radius}!")
    return samples

samples = checkedPoissonDiskSampling(0.35, 10)
for sample in samples:
    print(f"vec3({sample[0]}, {sample[1]}, {sample[2]}),")

