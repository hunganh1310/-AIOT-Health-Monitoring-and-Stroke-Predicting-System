import pandas as pd
import numpy as np
import glob
from pathlib import Path
from scipy import signal
from scipy.stats import skew, kurtosis
from tqdm import tqdm

from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split, cross_val_score, StratifiedKFold
from sklearn.preprocessing import StandardScaler
from sklearn.impute import SimpleImputer
from sklearn.metrics import classification_report, confusion_matrix, roc_auc_score, roc_curve
import matplotlib.pyplot as plt
import seaborn as sns

def extract_features(ppg_signal, fs=125):
    """Simple and fast feature extraction from PPG signal"""
    features = {}
    
    # Basic statistical features
    features['mean'] = np.mean(ppg_signal)
    features['std'] = np.std(ppg_signal)
    features['min'] = np.min(ppg_signal)
    features['max'] = np.max(ppg_signal)
    features['range'] = features['max'] - features['min']
    features['skewness'] = skew(ppg_signal)
    features['kurtosis'] = kurtosis(ppg_signal)
    features['rms'] = np.sqrt(np.mean(ppg_signal**2))
    
    # Peak-based features
    try:
        peaks, _ = signal.find_peaks(ppg_signal, height=features['mean'], distance=fs//3)
        features['num_peaks'] = len(peaks)
        
        if len(peaks) > 1:
            rr_intervals = np.diff(peaks) / fs
            features['mean_hr'] = 60 / np.mean(rr_intervals)
            features['std_hr'] = np.std(60 / rr_intervals)
            features['rmssd'] = np.sqrt(np.mean(np.diff(rr_intervals)**2))
        else:
            features.update({'mean_hr': 0, 'std_hr': 0, 'rmssd': 0})
    except:
        features.update({'num_peaks': 0, 'mean_hr': 0, 'std_hr': 0, 'rmssd': 0})
    
    # Frequency domain features
    try:
        f, psd = signal.welch(ppg_signal, fs=fs, nperseg=min(256, len(ppg_signal)//4))
        
        lf_mask = (f >= 0.04) & (f < 0.15)
        hf_mask = (f >= 0.15) & (f < 0.4)
        
        features['lf_power'] = np.trapz(psd[lf_mask], f[lf_mask]) if np.any(lf_mask) else 0
        features['hf_power'] = np.trapz(psd[hf_mask], f[hf_mask]) if np.any(hf_mask) else 0
        features['lf_hf_ratio'] = features['lf_power'] / (features['hf_power'] + 1e-6)
        features['dominant_freq'] = f[np.argmax(psd)]
    except:
        features.update({'lf_power': 0, 'hf_power': 0, 'lf_hf_ratio': 0, 'dominant_freq': 0})
    
    # Other simple features
    features['zero_crossings'] = len(np.where(np.diff(np.sign(ppg_signal - features['mean'])))[0])
    features['energy'] = np.sum(ppg_signal**2)
    
    return features