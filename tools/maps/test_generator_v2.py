#!/usr/bin/env python3
"""Unit tests for deterministic v2 map generation."""

from __future__ import annotations

import unittest

import numpy as np

from generator_v2 import (
    correlated_gaussian,
    generate_map,
    los_probability,
)


class GeneratorV2Test(unittest.TestCase):
    def test_same_seed_is_identical(self) -> None:
        first = generate_map(
            "URBAN_MICROCELL", 2.38, 42, cell_number=33)
        second = generate_map(
            "URBAN_MICROCELL", 2.38, 42, cell_number=33)
        self.assertEqual(first, second)

    def test_different_seed_changes_realization(self) -> None:
        first = generate_map(
            "URBAN_MICROCELL", 2.38, 42, cell_number=33)
        second = generate_map(
            "URBAN_MICROCELL", 2.38, 43, cell_number=33)
        self.assertNotEqual(first["map"], second["map"])

    def test_grid_has_explicit_center(self) -> None:
        payload = generate_map(
            "RURAL_MACROCELL", 3.5, 42, cell_number=33)
        self.assertEqual(payload["cell_number"], 33)
        self.assertEqual(
            payload["metadata"]["grid_origin"],
            "explicit-center-cell")
        center = payload["map"][16][16]
        self.assertTrue(np.isfinite(center))

    def test_correlated_field_is_normalized(self) -> None:
        field = correlated_gaussian(
            np.random.default_rng(42), 65, 5.0, 10.0)
        self.assertAlmostEqual(float(np.mean(field)), 0.0, places=10)
        self.assertAlmostEqual(float(np.std(field)), 1.0, places=10)

    def test_uma_uses_ue_height_correction(self) -> None:
        distance = np.asarray([200.0])
        normal_ue = los_probability(
            "URBAN_MACROCELL", distance, 1.5)[0]
        high_ue = los_probability(
            "URBAN_MACROCELL", distance, 25.0)[0]
        self.assertLess(normal_ue, high_ue)
        self.assertAlmostEqual(normal_ue, 0.128047, places=5)


if __name__ == "__main__":
    unittest.main()
