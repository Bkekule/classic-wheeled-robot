#!/bin/bash
# Post-activation script for jazzy environment
# Comments out Sensors plugin in URDF and uncomments/activates in SDF world files

URDF_FILE="$PIXI_PROJECT_ROOT/.pixi/envs/jazzy/share/irobot_create_description/urdf/create3.urdf.xacro"
WORLD_DIR="$PIXI_PROJECT_ROOT/.pixi/envs/jazzy/share/turtlebot4_gz_bringup/worlds"

echo "Running post-activation jazzy environment setup..."

# Comment out Sensors plugin in URDF file
if [ -f "$URDF_FILE" ]; then
    echo "Modifying $(basename $URDF_FILE)..."
    if grep -q 'gz::sim::systems::Sensors' "$URDF_FILE"; then
        awk '
            /<plugin name="gz::sim::systems::Sensors"/,/<\/plugin>/ {
                print "    <!--" $0 " -->"
                next
            }
            { print }
        ' "$URDF_FILE" > "$URDF_FILE.tmp" && mv "$URDF_FILE.tmp" "$URDF_FILE"
        echo "  ✓ Commented out Sensors plugin"
    fi
else
    echo "  ⚠ URDF file not found: $URDF_FILE"
fi

# Process world files
if [ -d "$WORLD_DIR" ]; then
    for world_file in "$WORLD_DIR"/*.sdf; do
        if [ -f "$world_file" ]; then
            echo "Modifying $(basename $world_file)..."

            # Uncomment the Sensors plugin block and add render engine settings
            awk '
                BEGIN { in_plugin = 0; done = 0 }
                /<!--.*<plugin name="gz::sim::systems::Sensors"/ {
                    in_plugin = 1
                    # Remove comment markers from the line
                    gsub(/^[ \t]*<!--/, "    ")
                    print
                    next
                }
                in_plugin && /-->[ \t]*$/ {
                    in_plugin = 0
                    # Remove the closing comment marker
                    gsub(/[ \t]*-->[ \t]*$/, "")
                    print
                    # Add render engine settings before </plugin>
                    if (!done && $0 ~ /<\/plugin>/) {
                        print "      <render_engine>ogre2</render_engine>"
                        print "      <render_engine_plugin>gz-rendering-ogre2</render_engine_plugin>"
                        done = 1
                    }
                    next
                }
                in_plugin && /<\/plugin>/ {
                    if (!done) {
                        print "      <render_engine>ogre2</render_engine>"
                        print "      <render_engine_plugin>gz-rendering-ogre2</render_engine_plugin>"
                        done = 1
                    }
                    print
                    in_plugin = 0
                    next
                }
                { print }
            ' "$world_file" > "$world_file.tmp" && mv "$world_file.tmp" "$world_file"
            echo "  ✓ Updated Sensors plugin"
        fi
    done
else
    echo "  ⚠ World directory not found: $WORLD_DIR"
fi

echo "Post-activation setup completed!"
