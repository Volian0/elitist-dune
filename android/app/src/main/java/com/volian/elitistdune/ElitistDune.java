package com.volian.elitistdune;

import org.libsdl.app.SDLActivity;

import android.os.Bundle;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdListener;
import android.widget.FrameLayout;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;

public class ElitistDune extends SDLActivity {
    AdView adView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        MobileAds.initialize(this, initializationStatus -> {});

        // Create a new ad view.
        adView = new AdView(this);
        adView.setAdUnitId("ca-app-pub-3940256099942544/9214589741");
        // Request a large anchored adaptive banner with a width of 360.
        adView.setAdSize(AdSize.BANNER);

        // Replace ad container with new ad view.
        FrameLayout layout = (FrameLayout) findViewById(android.R.id.content);

        FrameLayout.LayoutParams params =
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.WRAP_CONTENT,
                        FrameLayout.LayoutParams.WRAP_CONTENT);

        params.gravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;

        layout.addView(adView, params);

        AdRequest adRequest = new AdRequest.Builder().build();
        adView.loadAd(adRequest);
        adView.setAdListener(new AdListener() {
            @Override
            public void onAdLoaded() {
                resizeWindow();
            }
        });
    }

    private void resizeWindow()
    {
        //ViewGroup.LayoutParams gameLayoutParams = mSurface.getLayoutParams();
        //        gameLayoutParams.height = mSurface.getHeight() - adView.getAdSize().getHeightInPixels(this);
       // mSurface.setPadding(0, adView.getAdSize().getHeightInPixels(this), 0, 0);

        ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mSurface.getLayoutParams();
        params.topMargin = adView.getAdSize().getHeightInPixels(this); // margin in pixels

        mSurface.setLayoutParams(params);

        //mSurface.setLayoutParams(gameLayoutParams);
    }

    @Override
    protected void onPause() {
        if (adView != null) adView.pause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (adView != null) adView.resume();
    }

    @Override
    protected void onDestroy() {
        if (adView != null) adView.destroy();
        super.onDestroy();
    }

}
 