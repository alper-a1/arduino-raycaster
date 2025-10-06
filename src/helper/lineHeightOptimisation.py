"""
This File is a quick AI refactoring of the original lineHeightOptimisation.py script to clean it up and make it more understandable.
No logic has been changed, except for minor improvements in readability and structure.

Analyzes and optimizes a fixed-point line height calculation for a raycaster engine.

This script serves two purposes, controlled by the MODE variable:
1.  'ANALYZE': Plots the performance of the line height approximation against the exact
    calculation, showing the absolute error across a range of distances. 
    
2.  'OPTIMIZE': Runs a search to find the optimal value for the first element of the
    lookup table (lhTableQ78[0]). It plots the maximum and average error for each
    tested value to find the one that minimizes the maximum error, which is critical
    for reducing visual artifacts. 
    
3. 'OPTIMAL_ANALYSIS': Runs the analysis plots using the optimal value found from the
    optimization step.
"""

import matplotlib.pyplot as plt
import numpy as np

# --- Script Configuration ---
# Set the mode for the script.
# 'ANALYZE': Plots the line height approximation vs. the exact calculation.
# 'OPTIMIZE': Finds and plots the best value for lhTableQ78[0].
# 'OPTIMAL_ANALYSIS': Runs analysis plots using the optimal value found.
MODE = 'ANALYZE'

# --- Constants and Lookup Table Generation ---
SCREEN_HEIGHT = 128
MAX_PERPWALLDIST_INT = 12

def generate_lookup_table():
    """
    Precomputes the line height lookup table (lhTableQ78) for integer perpendicular
    wall distances. The table stores values in Q7.8 fixed-point format.
    """
    table = []
    for i in range(1, MAX_PERPWALLDIST_INT + 1):
        line_height = SCREEN_HEIGHT / i
        # Convert to Q7.8 fixed-point format
        iq78 = int(line_height * 256 + 0.5)
        table.append(iq78)
    return table

# Initialize the global lookup table
lhTableQ78 = generate_lookup_table()

# --- Core Calculation Functions (Preserving Original Logic) ---

def approximate_division_q78(perpwalldist_q78: int) -> int:
    """
    Approximates `SCREEN_HEIGHT / perpwalldist` using the precomputed lookup table
    and linear interpolation. All calculations are in Q7.8 fixed-point.

    Args:
        perpwalldist_q78: The perpendicular wall distance in Q7.8 format.

    Returns:
        The approximate line height in Q7.8 format.
    """
    int_part = perpwalldist_q78 >> 8
    frac_part = perpwalldist_q78 & 0xFF

    if int_part == 0:
        # Distance < 1.0, wall is very close. Return max height value.
        return 32767
    elif int_part > MAX_PERPWALLDIST_INT:
        int_part = MAX_PERPWALLDIST_INT

    # Get the base value from the lookup table.
    # The table is 0-indexed, so we subtract 1.
    base_val = lhTableQ78[int_part - 1]

    if frac_part == 0:
        # No fractional part, so no interpolation needed.
        return base_val

    # Perform linear interpolation.
    # Note: This will cause an index out of bounds if int_part is MAX_PERPWALLDIST_INT
    # and frac_part is non-zero. 
    next_val = lhTableQ78[int_part]
    diff = next_val - base_val
    interp = (diff * frac_part) >> 8
    return base_val + interp

def get_approx_line_height_int(perpwalldist_q78: int) -> int:
    """
    Calculates the approximate integer line height.
    """
    return approximate_division_q78(perpwalldist_q78) >> 8

def get_exact_line_height_int(perpwalldist_q78: int) -> int:
    """
    Calculates the 'exact' integer line height using fixed-point math.
    """
    if perpwalldist_q78 == 0:
        return SCREEN_HEIGHT
    exact_lh = (8388607 // perpwalldist_q78) >> 8
    # Clamp the value to screen height, as in the original test code.
    return min(exact_lh, SCREEN_HEIGHT)

# --- Plotting and Analysis Functions ---

def run_analysis_plots():
    """
    Generates and displays plots comparing the approximate line height to the exact
    calculation, along with the absolute error.
    """
    approx_list, exact_list, dist_float_list, error_list = [], [], [], []

    for dist_q78 in range(1, MAX_PERPWALLDIST_INT * 256, 5):
        approx_lh = get_approx_line_height_int(dist_q78)
        exact_lh = get_exact_line_height_int(dist_q78)

        approx_list.append(approx_lh)
        exact_list.append(exact_lh)
        dist_float_list.append(dist_q78 / 256.0)
        error_list.append(abs(approx_lh - exact_lh))

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), constrained_layout=True)
    fig.suptitle("Line Height Calculation Analysis", fontsize=16)

    ax1.set_title("Line Height vs. PerpWallDist")
    ax1.plot(dist_float_list, approx_list, label="Approximate")
    ax1.plot(dist_float_list, exact_list, label="Exact")
    ax1.set_xlabel("PerpWallDist (float)")
    ax1.set_ylabel("Line Height (pixels)")
    ax1.legend()
    ax1.grid()

    ax2.set_title("Absolute Error vs. PerpWallDist")
    ax2.plot(dist_float_list, error_list)
    ax2.set_xlabel("PerpWallDist (float)")
    ax2.set_ylabel("Absolute Error (pixels)")
    ax2.grid()

    plt.show()


def run_optimization_plots():
    """
    Finds the optimal value for lhTableQ78[0] by minimizing the maximum error and
    plots the results.
    """
    # Test distances from 1.0 to 2.0 (in Q7.8 format)
    pwd_test_vals = range(256, 513)
    # Range of values to test for lhTableQ78[0]
    lhtable0_vals = range(28000, 32768, 10)

    max_lh_errors, average_lh_errors = [], []

    for val in lhtable0_vals:
        lhTableQ78[0] = val
        errors = []
        for pwd in pwd_test_vals:
            approx_lh = get_approx_line_height_int(pwd)
            exact_lh = get_exact_line_height_int(pwd)
            errors.append(abs(approx_lh - exact_lh))
        max_lh_errors.append(np.max(errors))
        average_lh_errors.append(np.mean(errors))

    # Find the lhTableQ78[0] value that results in the minimum maximum error
    min_max_error_index = np.argmin(max_lh_errors)
    optimal_val = lhtable0_vals[min_max_error_index]
    min_max_error_val = max_lh_errors[min_max_error_index]
    avg_error_at_optimal = average_lh_errors[min_max_error_index]

    print(f"Optimal lhTableQ78[0] found: {optimal_val}")
    print(f"  - Minimized Max Error: {min_max_error_val:.2f} pixels")
    print(f"  - Average Error at this value: {avg_error_at_optimal:.2f} pixels")

    # --- Plotting ---
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), constrained_layout=True)
    fig.suptitle("Optimization of lhTableQ78[0]", fontsize=16)

    # Plot Max Error
    ax1.set_title("Max Line Height Error vs. lhTableQ78[0] Value")
    ax1.plot(lhtable0_vals, max_lh_errors, label="Max Error")
    ax1.plot(optimal_val, min_max_error_val, 'ro', label=f"Optimal: {optimal_val}")
    ax1.set_xlabel("Value for lhTableQ78[0]")
    ax1.set_ylabel("Max Absolute Error (pixels)")
    ax1.legend()
    ax1.grid()

    # Plot Average Error
    ax2.set_title("Average Line Height Error vs. lhTableQ78[0] Value")
    ax2.plot(lhtable0_vals, average_lh_errors, label="Average Error")
    ax2.plot(optimal_val, avg_error_at_optimal, 'ro', label=f"Avg Error at Optimal: {avg_error_at_optimal:.2f}")
    ax2.set_xlabel("Value for lhTableQ78[0]")
    ax2.set_ylabel("Average Absolute Error (pixels)")
    ax2.legend()
    ax2.grid()

    plt.show()


# --- Main Execution Block ---

if __name__ == "__main__":
    if MODE == 'ANALYZE':
        run_analysis_plots()
    elif MODE == 'OPTIMIZE':
        run_optimization_plots()
    elif MODE == 'OPTIMAL_ANALYSIS':
        # Set the optimal value found from previous optimization
        lhTableQ78[0] = 30840  
        run_analysis_plots()
    else:
        print(f"Invalid MODE: '{MODE}'. Please choose 'ANALYZE' or 'OPTIMIZE'.")

