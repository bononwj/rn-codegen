import React from 'react';
import {
  SafeAreaView,
  StyleSheet,
  Text,
  TextInput,
  Button,
  Alert,
} from 'react-native';

import NativeLocalStorage from './src/specs/NativeLocalStorage';
import NativeSampleModule from './src/specs/NativeSampleModule';
import WebViewNativeComponent from './src/specs/WebViewNativeComponent';

const EMPTY = '<empty>';

function App(): React.JSX.Element {
  const [value, setValue] = React.useState<string | null>(null);

  const [editingValue, setEditingValue] = React.useState<string | null>(null);

  React.useEffect(() => {
    const storedValue = NativeLocalStorage?.getItem('myKey');
    setValue(storedValue ?? '');
  }, []);

  function saveValue() {
    NativeLocalStorage?.setItem(editingValue ?? EMPTY, 'myKey');
    setValue(editingValue);
  }

  function clearAll() {
    NativeLocalStorage?.clear();
    setValue('');
  }

  function deleteValue() {
    NativeLocalStorage?.removeItem('myKey');
    setValue('');
  }

  function reverseString() {
    const reversedString = NativeSampleModule?.reverseString(
      editingValue ?? '',
    );
    setValue(reversedString);
  }

  return (
    <SafeAreaView style={{ flex: 1, paddingTop: 90 }}>
      <Text style={styles.text}>
        Current stored value is: {value ?? 'No Value'}
      </Text>
      <TextInput
        placeholder="Enter the text you want to store"
        style={styles.textInput}
        onChangeText={setEditingValue}
      />
      <Button title="Save" onPress={saveValue} />
      <Button title="Delete" onPress={deleteValue} />
      <Button title="Clear" onPress={clearAll} />
      <Button title="Reverse String" onPress={reverseString} />
      <WebViewNativeComponent
        sourceURL="https://react.dev/"
        style={{
          width: '100%',
          height: 200,
          borderColor: 'black',
          borderWidth: 1,
          paddingLeft: 5,
          paddingRight: 5,
          borderRadius: 5,
        }}
        onScriptLoaded={() => {
          Alert.alert('Page Loaded');
        }}
      />
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  text: {
    margin: 10,
    fontSize: 20,
  },
  textInput: {
    margin: 10,
    height: 40,
    borderColor: 'black',
    borderWidth: 1,
    paddingLeft: 5,
    paddingRight: 5,
    borderRadius: 5,
  },
});

export default App;
