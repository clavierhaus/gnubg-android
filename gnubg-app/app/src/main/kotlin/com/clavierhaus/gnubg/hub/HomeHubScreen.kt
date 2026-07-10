package com.clavierhaus.gnubg.hub

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Icon
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
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

        // The same gear as in-game (Icons.Filled.Settings), in the corner the eye
        // already goes to. It replaces the "Options" entry: a settings gear is a
        // convention, and the entry it displaces was the one item in the menu that
        // was not a destination.
        Icon(
            imageVector = Icons.Filled.Settings,
            contentDescription = "Options",
            tint = Color.White,
            modifier = Modifier
                .align(Alignment.TopStart)
                .padding(start = 48.dp, top = 30.dp)
                .size(36.dp)
                .clickable(onClick = onOptions)
        )

        BasicText(
            text = "GNU Backgammon",
            style = HomeTitleStyle,
            modifier = Modifier
                .align(Alignment.TopStart)
                .padding(start = 48.dp, top = 96.dp)
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

private val HomeTitleStyle = TextStyle(
    color = Color.White,
    fontSize = 34.sp,
    fontWeight = FontWeight.SemiBold,
    shadow = Shadow(
        color = Color.Black,
        offset = Offset(2f, 2f),
        blurRadius = 8f
    )
)

private val HomeEntryStyle = TextStyle(
    color = Color.White,
    fontSize = 36.sp,
    fontWeight = FontWeight.Medium,
    shadow = Shadow(
        color = Color.Black,
        offset = Offset(2f, 2f),
        blurRadius = 8f
    )
)

private val HomeSecondaryStyle = TextStyle(
    color = Color.White,
    fontSize = 26.sp,
    fontWeight = FontWeight.Medium,
    shadow = Shadow(
        color = Color.Black,
        offset = Offset(2f, 2f),
        blurRadius = 8f
    )
)
