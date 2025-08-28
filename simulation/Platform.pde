/**
 * Represents a 6-DOF (Degrees of Freedom) Stewart Platform.
 * This class calculates the inverse kinematics: given a desired translation (x, y, z)
 * and rotation (roll, pitch, yaw) of the top platform, it computes the
 * required angles (alpha) for the six servo motors at the base.
 * This code is designed for use with the Processing framework.
 */
class Platform {
  // --- STATE VECTORS ---
  private PVector translation;     // Desired translation (x, y, z) of the platform.
  private PVector rotation;        // Desired rotation (x=roll, y=pitch, z=yaw) of the platform.
  private PVector initialHeight;   // The default height of the platform when at rest.

  // --- GEOMETRY VECTORS ---
  private PVector[] baseJoint;     // (b_i) The fixed positions of the servo motors on the base.
  private PVector[] platformJoint; // (p_i) The positions of the leg attachment points on the top platform, in its own local coordinate system.
  private PVector[] q;             // The calculated world-space coordinates of the platform joints after translation and rotation.
  private PVector[] l;             // The calculated vectors for each leg, stretching from a base joint (b_i) to a platform joint (q_i).
  private PVector[] A;             // The calculated world-space coordinates of the end of each servo horn.

  // --- OUTPUT & DIMENSIONS ---
  private float[] alpha;           // (α_i) The calculated angles for each of the 6 servo motors.
  private float[] alphaHome;       // The stored "home" or "neutral" angles.
  private float baseRadius, platformRadius, hornLength, legLength; // Physical dimensions of the platform components.

  // --- HARDWARE CONSTANTS: GEOMETRY ---
  private final float baseAngles[] = {
    0.0f, 60.0f, 120.0f, 180.0f, 240.0f, 300.0f
  };
  private final float platformAngles[] = {
    334.1f, 79.9f, 94.1f, 199.9f, 214.1f, 319.9f
  };
  private final float beta[] = {
    2 * PI / 3, -PI / 3, 4 * PI / 3, PI / 3, 0, PI
  };

  // --- HARDWARE CONSTANTS: PHYSICAL DIMENSIONS (in millimeters) ---
  private final float SCALE_INITIAL_HEIGHT = 110;
  private final float SCALE_BASE_RADIUS = 65;
  private final float SCALE_PLATFORM_RADIUS = 55;
  private final float SCALE_HORN_LENGTH = 23;
  private final float SCALE_LEG_LENGTH = 120;

  /**
   * Constructor for the Platform class.
   */
  public Platform(float s) {
    translation = new PVector();
    initialHeight = new PVector(0, 0, s * SCALE_INITIAL_HEIGHT);
    rotation = new PVector();
    baseJoint = new PVector[6];
    platformJoint = new PVector[6];
    alpha = new float[6];
    alphaHome = new float[6];
    q = new PVector[6];
    l = new PVector[6];
    A = new PVector[6];
    baseRadius = s * SCALE_BASE_RADIUS;
    platformRadius = s * SCALE_PLATFORM_RADIUS;
    hornLength = s * SCALE_HORN_LENGTH;
    legLength = s * SCALE_LEG_LENGTH;

    for (int i = 0; i < 6; i++) {
      float mx = baseRadius * cos(radians(baseAngles[i]));
      float my = baseRadius * sin(radians(baseAngles[i]));
      baseJoint[i] = new PVector(mx, my, 0);
    }

    for (int i = 0; i < 6; i++) {
      float mx = platformRadius * cos(radians(platformAngles[i]));
      float my = platformRadius * sin(radians(platformAngles[i]));
      platformJoint[i] = new PVector(mx, my, 0);
      q[i] = new PVector(0, 0, 0);
      l[i] = new PVector(0, 0, 0);
      A[i] = new PVector(0, 0, 0);
    }
    
    calcQ();
    calcAlpha(); 
    System.arraycopy(alpha, 0, alphaHome, 0, 6);
  }

  /**
   * Applies a new translation and rotation to the platform.
   */
  public void applyTranslationAndRotation(PVector t, PVector r) {
    rotation.set(r);
    translation.set(t);
    calcQ();
    calcAlpha();
  }

  /**
   * INVERSE KINEMATICS - STEP 1: Calculates final world-space positions (q).
   */
  private void calcQ() {
    for (int i = 0; i < 6; i++) {
      q[i].x = cos(rotation.z) * cos(rotation.y) * platformJoint[i].x +
        (-sin(rotation.z) * cos(rotation.x) + cos(rotation.z) * sin(rotation.y) * sin(rotation.x)) * platformJoint[i].y +
        (sin(rotation.z) * sin(rotation.x) + cos(rotation.z) * sin(rotation.y) * cos(rotation.x)) * platformJoint[i].z;
      q[i].y = sin(rotation.z) * cos(rotation.y) * platformJoint[i].x +
        (cos(rotation.z) * cos(rotation.x) + sin(rotation.z) * sin(rotation.y) * sin(rotation.x)) * platformJoint[i].y +
        (-cos(rotation.z) * sin(rotation.x) + sin(rotation.z) * sin(rotation.y) * cos(rotation.x)) * platformJoint[i].z;
      q[i].z = -sin(rotation.y) * platformJoint[i].x +
        cos(rotation.y) * sin(rotation.x) * platformJoint[i].y +
        cos(rotation.y) * cos(rotation.x) * platformJoint[i].z;
      q[i].add(PVector.add(translation, initialHeight));
      l[i] = PVector.sub(q[i], baseJoint[i]);
    }
  }

  /**
   * INVERSE KINEMATICS - STEP 2: Calculates required servo angles (alpha).
   */
  private void calcAlpha() {
    for (int i = 0; i < 6; i++) {
      float L = l[i].magSq() - (legLength * legLength) + (hornLength * hornLength);
      float M = 2 * hornLength * (q[i].z - baseJoint[i].z);
      float N = 2 * hornLength * (cos(beta[i]) * (q[i].x - baseJoint[i].x) + sin(beta[i]) * (q[i].y - baseJoint[i].y));
      alpha[i] = asin(L / sqrt(M * M + N * N)) - atan2(N, M);
      A[i].set(hornLength * cos(alpha[i]) * cos(beta[i]) + baseJoint[i].x, 
               hornLength * cos(alpha[i]) * sin(beta[i]) + baseJoint[i].y, 
               hornLength * sin(alpha[i]) + baseJoint[i].z);
    }
  }
  
  /**
   * Returns the true physical angles (in radians).
   */
  public float[] getAlpha() {
    return alpha;
  }
  
  /**
   * Returns the remapped servo command angles (0-180 degrees).
   */
  public float[] getServoAngles() {
    float[] servoAngles = new float[6];
    for (int i = 0; i < 6; i++) {
      float delta_rad = alpha[i] - alphaHome[i];
      float delta_deg = degrees(delta_rad);
      float finalAngle = 60.0f + delta_deg;
      servoAngles[i] = constrain(finalAngle, 0, 180);
    }
    return servoAngles;
  }

  /**
   * Draws a dotted line in 3D space.
   */
  void drawDottedLine(PVector start, PVector end, float dashLength, float gapLength) {
    PVector lineVec = PVector.sub(end, start);
    float lineLength = lineVec.mag();
    PVector direction = lineVec.normalize();
    for (float d = 0; d < lineLength; d += (dashLength + gapLength)) {
      PVector dashStart = PVector.mult(direction, d);
      dashStart.add(start);
      float currentDashLength = min(dashLength, lineLength - d);
      PVector dashEnd = PVector.mult(direction, currentDashLength);
      dashEnd.add(dashStart);
      line(dashStart.x, dashStart.y, dashStart.z, dashEnd.x, dashEnd.y, dashEnd.z);
    }
  }

  /**
   * Draws the entire Stewart Platform and all visualizations.
   */
  public void draw() {
    // --- MEASUREMENT VISUALIZATION ---
    strokeWeight(1);
    fill(255, 200, 0); // Bright yellow for labels
    stroke(255, 200, 0);
    drawDottedLine(new PVector(0, 0, 0), baseJoint[0], 5, 5);
    text("Base Radius: " + baseRadius, baseJoint[0].x / 2, baseJoint[0].y / 2, 10);
    drawDottedLine(new PVector(0, 0, 0), initialHeight, 5, 5);
    text("Initial Height: " + initialHeight.z, 5, 5, initialHeight.z / 2);
    
    // --- MAIN DRAWING ---
    noStroke();
    fill(128);
    beginShape();
    for (int i = 0; i < 6; i++) {
      vertex(baseJoint[i].x, baseJoint[i].y);
    }
    endShape(CLOSE);

    // Get the final servo command angles once for efficiency.
    float[] servoCommandAngles = getServoAngles();

    // Loop through each of the 6 leg assemblies.
    for (int i = 0; i < 6; i++) {
      pushMatrix();
      translate(baseJoint[i].x, baseJoint[i].y, baseJoint[i].z);
      noStroke();
      fill(0);
      ellipse(0, 0, 5, 5); // Draw the base joint.
      
      // --- MODIFIED TEXT DISPLAY ---
      // Format a string to show both the command and the physical angle.
      String label = String.format("%.1f", servoCommandAngles[i]);
      fill(255,0, 0); // Yellow for the label
      text(label, 15, 0, 5); // Adjusted position for better visibility
      
      fill(255, 0, 0); // Red for motor labels
      text("M" + (i + 1), 10, -10, 10);
      popMatrix();

      // Draw the servo horn.
      stroke(245);
      line(baseJoint[i].x, baseJoint[i].y, baseJoint[i].z, A[i].x, A[i].y, A[i].z);

      // Calculate final rod position to ensure correct length.
      PVector rod = PVector.sub(q[i], A[i]);
      rod.setMag(legLength);
      rod.add(A[i]);

      // Draw the main leg.
      stroke(100);
      strokeWeight(3);
      line(A[i].x, A[i].y, A[i].z, rod.x, rod.y, rod.z);
      
      // Draw Horn & Leg measurements for the first leg only to avoid clutter.
      if (i == 0) {
        stroke(255, 200, 0);
        strokeWeight(1);
        PVector hornMid = PVector.add(baseJoint[i], A[i]).div(2);
        text("Horn: " + hornLength, hornMid.x, hornMid.y, hornMid.z);
        PVector legMid = PVector.add(A[i], rod).div(2);
        text("Leg: " + legLength, legMid.x + 10, legMid.y, legMid.z);
      }
    }

    // Draw the joints on the top platform.
    for (int i = 0; i < 6; i++) {
      pushMatrix();
      translate(q[i].x, q[i].y, q[i].z);
      noStroke();
      fill(0);
      ellipse(0, 0, 5, 5);
      popMatrix();
      stroke(100);
      strokeWeight(1);
      line(baseJoint[i].x, baseJoint[i].y, baseJoint[i].z, q[i].x, q[i].y, q[i].z);
    }

    // --- SANITY CHECK & PLATFORM RADIUS VISUALIZATION ---
    pushMatrix();
    translate(initialHeight.x, initialHeight.y, initialHeight.z);
    translate(translation.x, translation.y, translation.z);
    rotateZ(rotation.z);
    rotateY(rotation.y);
    rotateX(rotation.x);
    
    // Draw Platform Radius.
    stroke(255, 200, 0);
    strokeWeight(1);
    fill(255, 200, 0);
    PVector platformRadiusVec = new PVector(platformRadius, 0, 0);
    drawDottedLine(new PVector(0,0,0), platformRadiusVec, 5, 5);
    text("Platform Radius: " + platformRadius, platformRadius / 2, 15, 0);
    
    // Draw the outline of the top platform.
    stroke(245);
    noFill();
    ellipse(0, 0, 2 * platformRadius, 2 * platformRadius);
    popMatrix();
  }
}
