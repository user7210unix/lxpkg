#!/bin/sh
# Order packages based on dependencies
pkg_order() {
    unset order deps # Clear globals for fresh run
    for pkg; do
        pkg_depends "$pkg" raw
    done

    # Re-order based on the collected 'deps'
    # This might need a more sophisticated topological sort for complex graphs.
    # For now, it assumes simple ordering where direct dependencies are listed first.
    local ordered_pkgs=""
    for d in $deps; do
        if ! contains "$ordered_pkgs" "$d"; then
            ordered_pkgs="$ordered_pkgs $d"
        fi
    done
    printf '%s' "$ordered_pkgs"
}

# Resolve dependencies for a package
pkg_depends() {
    local pkg="$1" type="$2" parent_stack="$3" dep_type="$4"

    # If already processed or part of a circular dependency, return
    contains "$deps" "$pkg" && return 0
    contains "$parent_stack" "$pkg" && { warn "Circular dependency detected involving $pkg"; return 0; } # Warn but don't die yet

    local current_deps_file="$repo_dir/depends" # Make sure repo_dir is set by pkg_find earlier
    pkg_find "$pkg" # Ensure repo_dir is set for the current package

    if [ -f "$repo_dir/depends" ]; then
        while read -r dep make_type || [ -n "$dep" ]; do
            [ "${dep##\#*}" ] || continue
            # Handle alternative dependencies (e.g., a|b|c)
            local resolved_dep=""
            IFS='|' read -ra ALTS <<< "$dep"
            for alt in "${ALTS[@]}"; do
                if pkg_is_installed "$alt" || pkg_check_system "$alt" >/dev/null 2>&1 || pkg_find "$alt" >/dev/null 2>&1; then
                    resolved_dep="$alt"
                    break
                fi
            done

            if [ -z "$resolved_dep" ]; then
                warn "Could not resolve dependency for $pkg: $dep (from alternatives: ${ALTS[*]})"
                prompt "Missing dependency for $pkg: $dep. Continue?"
                continue
            fi
            
            # Recurse for dependencies, extending the parent stack
            pkg_depends "$resolved_dep" "" "$parent_stack $pkg" "$make_type"
        done < "$repo_dir/depends"
    fi

    # Add the current package to the global deps list if it's not a make dependency
    if [ "$type" != "make" ]; then
        deps="$deps $pkg"
    fi
}

# Resolve dependencies (main entry point for installation)
pkg_resolve_deps() {
    unset _seen_deps _dep_stack deps # Reset globals for a fresh resolution
    log "Resolving dependencies for $@..."
    local pkgs_to_install=("$@")
    local resolved_order=""

    # First pass: collect all direct and transitive dependencies in order
    for pkg in "${pkgs_to_install[@]}"; do
        pkg_depends "$pkg" "" "$pkg" "" # Start recursion for each requested package
        # After pkg_depends runs, 'deps' global will contain the ordered dependencies
    done

    # Deduplicate and sort the 'deps' list (a simple topological sort based on order of appearance)
    local unique_deps=""
    for d in $deps; do
        if ! contains "$unique_deps" "$d"; then
            unique_deps="$unique_deps $d"
        fi
    done
    deps="$unique_deps" # Update the global deps

    log "Calculated installation order: $deps"

    # Second pass: Install missing dependencies
    for dep in $deps; do
        if pkg_is_installed "$dep"; then
            read -r ver rel < "$sys_db/$dep/version" 2>/dev/null || { ver="unknown"; rel="unknown"; }
            printf '%b * %b%-20s %bStatus:%b %s %bVersion:%b %s-%s\n' "$c_grey" "$c_cyan" "$dep" "$c_grey" "$c_green" "Installed" "$c_grey" "$c_green" "$ver" "$rel"
        elif pkg_check_system "$dep"; then
            printf '%b * %b%-20s %bStatus:%b %s\n' "$c_grey" "$c_cyan" "$dep" "$c_grey" "$c_green" "System-installed"
        elif ! pkg_find "$dep" "" "" "$LXPKG_PATH"; then
            printf '%b * %b%-20s %bStatus:%b %s\n' "$c_grey" "$c_cyan" "$dep" "$c_grey" "$c_red" "Missing"
            prompt "Dependency $dep not found in repositories. Continue without installation?"
            continue # Allow user to proceed even if dependency is missing
        else
            printf '%b * %b%-20s %bStatus:%b %s\n' "$c_grey" "$c_cyan" "$dep" "$c_grey" "$c_yellow" "Not installed"
            log "Installing missing dependency: $dep"
            pkg_install "$dep" # Directly call install for dependencies
        fi
    done

    # Reset globals after resolution
    unset _seen_deps _dep_stack
}

# Fix runtime dependencies (similar to resolve, but specifically for fixing installed packages)
pkg_fix_deps() {
    local pkg="$1"
    pkg_find "$pkg" # Set repo_dir for the given package

    [ -f "$repo_dir/depends" ] || { log "$pkg has no dependencies"; return 0; }
    
    # Filter out 'make' dependencies
    local runtime_deps=""
    while read -r dep make_type || [ -n "$dep" ]; do
        [ "${dep##\#*}" ] || continue
        if [ "$make_type" != "make" ]; then
            runtime_deps="$runtime_deps $dep"
        fi
    done < "$repo_dir/depends"

    if [ -n "$runtime_deps" ]; then
        log "Runtime dependencies for $c_cyan$pkg$c_reset:"
        for dep in $runtime_deps; do
            # Handle alternative dependencies for runtime (same logic as pkg_depends)
            local resolved_dep=""
            IFS='|' read -ra ALTS <<< "$dep"
            for alt in "${ALTS[@]}"; do
                if pkg_is_installed "$alt" || pkg_check_system "$alt" >/dev/null 2>&1 || pkg_find "$alt" >/dev/null 2>&1; then
                    resolved_dep="$alt"
                    break
                fi
            done

            if [ -z "$resolved_dep" ]; then
                printf '%b * %b%-20s %bStatus:%b %s\n' "$c_grey" "$c_cyan" "$dep" "$c_grey" "$c_red" "Missing"
                prompt "Dependency $dep not found in repositories. Continue?"
            elif pkg_is_installed "$resolved_dep"; then
                read -r ver rel < "$sys_db/$resolved_dep/version" 2>/dev/null || { ver="unknown"; rel="unknown"; }
                printf '%b * %b%-20s %bStatus:%b %s %bVersion:%b %s-%s\n' "$c_grey" "$c_cyan" "$resolved_dep" "$c_grey" "$c_green" "Installed" "$c_grey" "$c_green" "$ver" "$rel"
            elif pkg_check_system "$resolved_dep"; then
                printf '%b * %b%-20s %bStatus:%b %s\n' "$c_grey" "$c_cyan" "$resolved_dep" "$c_grey" "$c_green" "System-installed"
            else
                printf '%b * %b%-20s %bStatus:%b %s\n' "$c_grey" "$c_cyan" "$resolved_dep" "$c_grey" "$c_yellow" "Not installed"
                log "Installing runtime dependency $resolved_dep for $pkg"
                pkg_install "$resolved_dep"
            fi
        done
    else
        log "$pkg has no runtime dependencies"
    fi
}
