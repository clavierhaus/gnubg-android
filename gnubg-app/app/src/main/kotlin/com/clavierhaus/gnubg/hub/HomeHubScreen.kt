package com.clavierhaus.gnubg.hub

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.blur
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shadow
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.clavierhaus.gnubg.R

@Composable
fun HomeHubScreen(
    onPlay: () -> Unit,
    onAnalysePosition: () -> Unit,
    onReviewMatch: () -> Unit,
    onOptions: () -> Unit,
    onProfile: () -> Unit
) {
    Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
        Image(
            painter = painterResource(id = R.drawable.home_hub_background),
            contentDescription = null,
            // Fit preserves aspect (no distortion) and is device-independent;
            // wider screens get black bars left/right, taller ones top/bottom.
            modifier = Modifier.fillMaxSize(),
            contentScale = ContentScale.Fit
        )

        // The same gear as in-game (Icons.Filled.Settings), wearing the hub's own
        // treatment: white, over the same drop shadow the text carries.
        //
        // TextStyle.Shadow is not available to an Icon, so it is reproduced: a black
        // copy behind, displaced and blurred by the same amounts. Those amounts are
        // PIXELS in Shadow -- Offset(2f, 2f) and blurRadius = 8f -- so they are
        // converted through the density rather than guessed at in dp.
        val density = LocalDensity.current
        val shadowShift = with(density) { HOME_SHADOW_OFFSET_PX.toDp() }
        val shadowBlur  = with(density) { HOME_SHADOW_BLUR_PX.toDp() }

        Box(
            modifier = Modifier
                .align(Alignment.TopStart)
                // Room for the blur to spread without being clipped by the Box.
                .padding(start = 48.dp - shadowBlur, top = 24.dp - shadowBlur)
                .padding(shadowBlur)
        ) {
            Icon(
                imageVector = Icons.Filled.Settings,
                contentDescription = null,
                tint = Color.Black,
                modifier = Modifier
                    .offset(x = shadowShift, y = shadowShift)
                    .blur(shadowBlur)
                    .size(GEAR_SIZE)
            )
            Icon(
                imageVector = Icons.Filled.Settings,
                contentDescription = "Options",
                tint = Color.White,
                modifier = Modifier
                    .size(GEAR_SIZE)
                    .clickable(onClick = onOptions)
            )
        }

        BasicText(
            text = "GNU Backgammon",
            style = HomeTitleStyle,
            modifier = Modifier
                .align(Alignment.TopStart)
                .padding(start = 48.dp, top = 80.dp)
        )

        // padding, not offset. An offset composable keeps the layout slot its parent
        // allocated; if it is pushed outside the parent's bounds it stops receiving
        // pointer events, which is how the match-setup screen once grew a button that
        // could be seen but not tapped. padding moves the slot itself, so the question
        // does not arise.
        Column(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .padding(start = 64.dp)
        ) {
            HomeHubEntry("Play Tournament Match", onPlay)
            Spacer(modifier = Modifier.height(22.dp))
            // Second: the feature people still open XG Mobile for.
            HomeHubEntry("Analyse Position", onAnalysePosition)
            Spacer(modifier = Modifier.height(22.dp))
            // Third, now that it exists.
            HomeHubEntry("Review Match", onReviewMatch)
        }

        BasicText(
            text = "Profile",
            style = HomeSecondaryStyle,
            modifier = Modifier
                .align(Alignment.BottomEnd)
                .clickable(onClick = onProfile)
                .padding(end = 42.dp, bottom = 34.dp)
        )
    }
}

@Composable
private fun HomeHubEntry(
    label: String,
    onClick: () -> Unit
) {
    BasicText(
        text = label,
        style = HomeEntryStyle,
        modifier = Modifier
            .clickable(onClick = onClick)
            .padding(vertical = 4.dp)
    )
}

// The hub's drop shadow, in the units Shadow uses: pixels.
private const val HOME_SHADOW_OFFSET_PX = 2f
private const val HOME_SHADOW_BLUR_PX = 8f
private val GEAR_SIZE = 36.dp

private val HomeShadow = Shadow(
    color = Color.Black,
    offset = Offset(HOME_SHADOW_OFFSET_PX, HOME_SHADOW_OFFSET_PX),
    blurRadius = HOME_SHADOW_BLUR_PX
)

private val HomeTitleStyle = TextStyle(
    color = Color.White,
    fontSize = 34.sp,
    fontWeight = FontWeight.SemiBold,
    shadow = HomeShadow
)

private val HomeEntryStyle = TextStyle(
    color = Color.White,
    fontSize = 36.sp,
    fontWeight = FontWeight.Medium,
    shadow = HomeShadow
)

private val HomeSecondaryStyle = TextStyle(
    color = Color.White,
    fontSize = 26.sp,
    fontWeight = FontWeight.Medium,
    shadow = HomeShadow
)
